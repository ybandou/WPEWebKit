/*
 * Copyright (C) 2006, 2007, 2008, 2013 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "DataTransfer.h"

#include "CachedImage.h"
#include "CachedImageClient.h"
#include "DataTransferItemList.h"
#include "DragData.h"
#include "Editor.h"
#include "FileList.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "HTMLImageElement.h"
#include "Image.h"
#include "Pasteboard.h"
#include "StaticPasteboard.h"

namespace WebCore {

#if ENABLE(DRAG_SUPPORT)

class DragImageLoader final : private CachedImageClient {
    WTF_MAKE_NONCOPYABLE(DragImageLoader); WTF_MAKE_FAST_ALLOCATED;
public:
    explicit DragImageLoader(DataTransfer*);
    void startLoading(CachedResourceHandle<CachedImage>&);
    void stopLoading(CachedResourceHandle<CachedImage>&);

private:
    void imageChanged(CachedImage*, const IntRect*) override;
    DataTransfer* m_dataTransfer;
};

#endif

DataTransfer::DataTransfer(StoreMode mode, std::unique_ptr<Pasteboard> pasteboard, Type type)
    : m_storeMode(mode)
    , m_pasteboard(WTFMove(pasteboard))
#if ENABLE(DRAG_SUPPORT)
    , m_type(type)
    , m_dropEffect(ASCIILiteral("uninitialized"))
    , m_effectAllowed(ASCIILiteral("uninitialized"))
    , m_shouldUpdateDragImage(false)
#endif
{
#if !ENABLE(DRAG_SUPPORT)
    ASSERT_UNUSED(type, type != Type::DragAndDropData && type != Type::DragAndDropFiles);
#endif
}

Ref<DataTransfer> DataTransfer::createForCopyAndPaste(StoreMode mode)
{
    return adoptRef(*new DataTransfer(mode, mode == StoreMode::ReadWrite ? std::make_unique<StaticPasteboard>() : Pasteboard::createForCopyAndPaste()));
}

DataTransfer::~DataTransfer()
{
#if ENABLE(DRAG_SUPPORT)
    if (m_dragImageLoader && m_dragImage)
        m_dragImageLoader->stopLoading(m_dragImage);
#endif
}

bool DataTransfer::canReadTypes() const
{
    return m_storeMode == StoreMode::Readonly || m_storeMode == StoreMode::Protected || m_storeMode == StoreMode::ReadWrite;
}

bool DataTransfer::canReadData() const
{
    return m_storeMode == StoreMode::Readonly || m_storeMode == StoreMode::ReadWrite;
}

bool DataTransfer::canWriteData() const
{
    return m_storeMode == StoreMode::ReadWrite;
}

static String normalizeType(const String& type)
{
    if (type.isNull())
        return type;

    String lowercaseType = type.stripWhiteSpace().convertToASCIILowercase();
    if (lowercaseType == "text" || lowercaseType.startsWithIgnoringASCIICase("text/plain;"))
        return "text/plain";
    if (lowercaseType == "url" || lowercaseType.startsWithIgnoringASCIICase("text/uri-list;"))
        return "text/uri-list";
    if (lowercaseType.startsWithIgnoringASCIICase("text/html;"))
        return "text/html";

    return lowercaseType;
}

void DataTransfer::clearData(const String& type)
{
    if (!canWriteData())
        return;

    String normalizedType = normalizeType(type);
    if (normalizedType.isNull())
        m_pasteboard->clear();
    else
        m_pasteboard->clear(normalizedType);
    if (m_itemList)
        m_itemList->didClearStringData(normalizedType);
}

String DataTransfer::getData(const String& type) const
{
    if (!canReadData())
        return String();

#if ENABLE(DRAG_SUPPORT)
    if (forFileDrag() && m_pasteboard->readFilenames().size())
        return { };
#endif

    return m_pasteboard->readString(normalizeType(type));
}

void DataTransfer::setData(const String& type, const String& data)
{
    if (!canWriteData())
        return;

#if ENABLE(DRAG_SUPPORT)
    if (forFileDrag() && m_pasteboard->readFilenames().size())
        return;
#endif

    String normalizedType = normalizeType(type);
    m_pasteboard->writeString(normalizedType, data);
    if (m_itemList)
        m_itemList->didSetStringData(normalizedType);
}

DataTransferItemList& DataTransfer::items()
{
    if (!m_itemList)
        m_itemList = std::make_unique<DataTransferItemList>(*this);
    return *m_itemList;
}

Vector<String> DataTransfer::types() const
{
    if (!canReadTypes())
        return { };

    return m_pasteboard->types();
}

FileList& DataTransfer::files() const
{
    bool newlyCreatedFileList = !m_fileList;
    if (!m_fileList)
        m_fileList = FileList::create();

    if (!canReadData()) {
        m_fileList->clear();
        return *m_fileList;
    }

#if ENABLE(DRAG_SUPPORT)
    if (forDrag() && !forFileDrag()) {
        ASSERT(m_fileList->isEmpty());
        return *m_fileList;
    }
#endif

    if (newlyCreatedFileList) {
        for (auto& filename : m_pasteboard->readFilenames())
            m_fileList->append(File::create(filename));
    }
    return *m_fileList;
}

bool DataTransfer::hasFileOfType(const String& type)
{
    ASSERT_WITH_SECURITY_IMPLICATION(canReadTypes());

    for (auto& path : m_pasteboard->readFilenames()) {
        if (equalIgnoringASCIICase(File::contentTypeForFile(path), type))
            return true;
    }

    return false;
}

bool DataTransfer::hasStringOfType(const String& type)
{
    ASSERT_WITH_SECURITY_IMPLICATION(canReadTypes());

    return !type.isNull() && types().contains(type);
}

Ref<DataTransfer> DataTransfer::createForInputEvent(const String& plainText, const String& htmlText)
{
    auto pasteboard = std::make_unique<StaticPasteboard>();
    pasteboard->writeString(ASCIILiteral("text/plain"), plainText);
    pasteboard->writeString(ASCIILiteral("text/html"), htmlText);
    return adoptRef(*new DataTransfer(StoreMode::Readonly, WTFMove(pasteboard), Type::InputEvent));
}

#if !ENABLE(DRAG_SUPPORT)

String DataTransfer::dropEffect() const
{
    return ASCIILiteral("none");
}

void DataTransfer::setDropEffect(const String&)
{
}

String DataTransfer::effectAllowed() const
{
    return ASCIILiteral("uninitialized");
}

void DataTransfer::setEffectAllowed(const String&)
{
}

void DataTransfer::setDragImage(Element*, int, int)
{
}

#else

Ref<DataTransfer> DataTransfer::createForDrag()
{
    return adoptRef(*new DataTransfer(StoreMode::ReadWrite, Pasteboard::createForDragAndDrop(), Type::DragAndDropData));
}

Ref<DataTransfer> DataTransfer::createForDrop(StoreMode accessMode, const DragData& dragData)
{
    auto type = dragData.containsFiles() ? Type::DragAndDropFiles : Type::DragAndDropData;
    return adoptRef(*new DataTransfer(accessMode, Pasteboard::createForDragAndDrop(dragData), type));
}

void DataTransfer::setDragImage(Element* element, int x, int y)
{
    if (!forDrag() || !canWriteData())
        return;

    CachedImage* image = nullptr;
    if (is<HTMLImageElement>(element) && !element->isConnected())
        image = downcast<HTMLImageElement>(*element).cachedImage();

    m_dragLocation = IntPoint(x, y);

    if (m_dragImageLoader && m_dragImage)
        m_dragImageLoader->stopLoading(m_dragImage);
    m_dragImage = image;
    if (m_dragImage) {
        if (!m_dragImageLoader)
            m_dragImageLoader = std::make_unique<DragImageLoader>(this);
        m_dragImageLoader->startLoading(m_dragImage);
    }

    m_dragImageElement = image ? nullptr : element;

    updateDragImage();
}

void DataTransfer::updateDragImage()
{
    // Don't allow setting the image if we haven't started dragging yet; we'll rely on the dragging code
    // to install this drag image as part of getting the drag kicked off.
    if (!m_shouldUpdateDragImage)
        return;

    IntPoint computedHotSpot;
    auto computedImage = DragImage { createDragImage(computedHotSpot) };
    if (!computedImage)
        return;

    m_pasteboard->setDragImage(WTFMove(computedImage), computedHotSpot);
}

RefPtr<Element> DataTransfer::dragImageElement() const
{
    return m_dragImageElement;
}

#if !PLATFORM(MAC)

DragImageRef DataTransfer::createDragImage(IntPoint& location) const
{
    location = m_dragLocation;

    if (m_dragImage)
        return createDragImageFromImage(m_dragImage->image(), ImageOrientationDescription());

    if (m_dragImageElement) {
        if (Frame* frame = m_dragImageElement->document().frame())
            return createDragImageForNode(*frame, *m_dragImageElement);
    }

    // We do not have enough information to create a drag image, use the default icon.
    return nullptr;
}

#endif

DragImageLoader::DragImageLoader(DataTransfer* dataTransfer)
    : m_dataTransfer(dataTransfer)
{
}

void DragImageLoader::startLoading(CachedResourceHandle<WebCore::CachedImage>& image)
{
    // FIXME: Does this really trigger a load? Does it need to?
    image->addClient(*this);
}

void DragImageLoader::stopLoading(CachedResourceHandle<WebCore::CachedImage>& image)
{
    image->removeClient(*this);
}

void DragImageLoader::imageChanged(CachedImage*, const IntRect*)
{
    m_dataTransfer->updateDragImage();
}

static DragOperation dragOpFromIEOp(const String& operation)
{
    if (operation == "uninitialized")
        return DragOperationEvery;
    if (operation == "none")
        return DragOperationNone;
    if (operation == "copy")
        return DragOperationCopy;
    if (operation == "link")
        return DragOperationLink;
    if (operation == "move")
        return (DragOperation)(DragOperationGeneric | DragOperationMove);
    if (operation == "copyLink")
        return (DragOperation)(DragOperationCopy | DragOperationLink);
    if (operation == "copyMove")
        return (DragOperation)(DragOperationCopy | DragOperationGeneric | DragOperationMove);
    if (operation == "linkMove")
        return (DragOperation)(DragOperationLink | DragOperationGeneric | DragOperationMove);
    if (operation == "all")
        return DragOperationEvery;
    return DragOperationPrivate; // really a marker for "no conversion"
}

static const char* IEOpFromDragOp(DragOperation operation)
{
    bool isGenericMove = operation & (DragOperationGeneric | DragOperationMove);

    if ((isGenericMove && (operation & DragOperationCopy) && (operation & DragOperationLink)) || operation == DragOperationEvery)
        return "all";
    if (isGenericMove && (operation & DragOperationCopy))
        return "copyMove";
    if (isGenericMove && (operation & DragOperationLink))
        return "linkMove";
    if ((operation & DragOperationCopy) && (operation & DragOperationLink))
        return "copyLink";
    if (isGenericMove)
        return "move";
    if (operation & DragOperationCopy)
        return "copy";
    if (operation & DragOperationLink)
        return "link";
    return "none";
}

DragOperation DataTransfer::sourceOperation() const
{
    DragOperation operation = dragOpFromIEOp(m_effectAllowed);
    ASSERT(operation != DragOperationPrivate);
    return operation;
}

DragOperation DataTransfer::destinationOperation() const
{
    DragOperation operation = dragOpFromIEOp(m_dropEffect);
    ASSERT(operation == DragOperationCopy || operation == DragOperationNone || operation == DragOperationLink || operation == (DragOperation)(DragOperationGeneric | DragOperationMove) || operation == DragOperationEvery);
    return operation;
}

void DataTransfer::setSourceOperation(DragOperation operation)
{
    ASSERT_ARG(operation, operation != DragOperationPrivate);
    m_effectAllowed = IEOpFromDragOp(operation);
}

void DataTransfer::setDestinationOperation(DragOperation operation)
{
    ASSERT_ARG(operation, operation == DragOperationCopy || operation == DragOperationNone || operation == DragOperationLink || operation == DragOperationGeneric || operation == DragOperationMove || operation == (DragOperation)(DragOperationGeneric | DragOperationMove));
    m_dropEffect = IEOpFromDragOp(operation);
}

String DataTransfer::dropEffect() const
{
    return m_dropEffect == "uninitialized" ? ASCIILiteral("none") : m_dropEffect;
}

void DataTransfer::setDropEffect(const String& effect)
{
    if (!forDrag())
        return;

    if (effect != "none" && effect != "copy" && effect != "link" && effect != "move")
        return;

    // FIXME: The spec allows this in all circumstances. There is probably no value
    // in ignoring attempts to change it.
    if (!canReadTypes())
        return;

    m_dropEffect = effect;
}

String DataTransfer::effectAllowed() const
{
    return m_effectAllowed;
}

void DataTransfer::setEffectAllowed(const String& effect)
{
    if (!forDrag())
        return;

    // Ignore any attempts to set it to an unknown value.
    if (dragOpFromIEOp(effect) == DragOperationPrivate)
        return;

    if (!canWriteData())
        return;

    m_effectAllowed = effect;
}

#endif // ENABLE(DRAG_SUPPORT)

} // namespace WebCore
