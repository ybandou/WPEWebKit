/*
 * Copyright (C) 2011 Adobe Systems Incorporated. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "config.h"
#include "RenderFlowThread.h"

#include "HitTestRequest.h"
#include "HitTestResult.h"
#include "InlineElementBox.h"
#include "Node.h"
#include "PODIntervalTree.h"
#include "RenderBoxRegionInfo.h"
#include "RenderInline.h"
#include "RenderLayer.h"
#include "RenderLayerCompositor.h"
#include "RenderRegion.h"
#include "RenderTableCell.h"
#include "RenderTableSection.h"
#include "RenderTheme.h"
#include "RenderView.h"
#include "TransformState.h"
#include <wtf/StackStats.h>

namespace WebCore {

RenderFlowThread::RenderFlowThread(Document& document, RenderStyle&& style)
    : RenderBlockFlow(document, WTFMove(style))
    , m_currentRegionMaintainer(nullptr)
    , m_regionsInvalidated(false)
    , m_regionsHaveUniformLogicalWidth(true)
    , m_regionsHaveUniformLogicalHeight(true)
    , m_pageLogicalSizeChanged(false)
{
    setIsRenderFlowThread(true);
}

RenderStyle RenderFlowThread::createFlowThreadStyle(const RenderStyle* parentStyle)
{
    auto newStyle = RenderStyle::create();
    newStyle.inheritFrom(*parentStyle);
    newStyle.setDisplay(BLOCK);
    newStyle.setPosition(AbsolutePosition);
    newStyle.setZIndex(0);
    newStyle.setLeft(Length(0, Fixed));
    newStyle.setTop(Length(0, Fixed));
    newStyle.setWidth(Length(100, Percent));
    newStyle.setHeight(Length(100, Percent));
    newStyle.fontCascade().update(nullptr);
    return newStyle;
}

void RenderFlowThread::styleDidChange(StyleDifference diff, const RenderStyle* oldStyle)
{
    RenderBlockFlow::styleDidChange(diff, oldStyle);

    if (oldStyle && oldStyle->writingMode() != style().writingMode())
        invalidateRegions();
}

void RenderFlowThread::removeFlowChildInfo(RenderElement& child)
{
    if (is<RenderBlockFlow>(child))
        removeLineRegionInfo(downcast<RenderBlockFlow>(child));
    if (is<RenderBox>(child))
        removeRenderBoxRegionInfo(downcast<RenderBox>(child));
}

void RenderFlowThread::removeRegionFromThread(RenderRegion* renderRegion)
{
    ASSERT(renderRegion);
    m_regionList.remove(renderRegion);
}

void RenderFlowThread::invalidateRegions(MarkingBehavior markingParents)
{
    if (m_regionsInvalidated) {
        ASSERT(selfNeedsLayout());
        return;
    }

    m_regionRangeMap.clear();
    m_breakBeforeToRegionMap.clear();
    m_breakAfterToRegionMap.clear();
    if (m_lineToRegionMap)
        m_lineToRegionMap->clear();
    setNeedsLayout(markingParents);

    m_regionsInvalidated = true;
}

void RenderFlowThread::validateRegions()
{
    if (m_regionsInvalidated) {
        m_regionsInvalidated = false;
        m_regionsHaveUniformLogicalWidth = true;
        m_regionsHaveUniformLogicalHeight = true;

        if (hasRegions()) {
            LayoutUnit previousRegionLogicalWidth = 0;
            LayoutUnit previousRegionLogicalHeight = 0;
            bool firstRegionVisited = false;
            
            for (auto& region : m_regionList) {
                ASSERT(!region->needsLayout() || region->isRenderRegionSet());

                region->deleteAllRenderBoxRegionInfo();

                LayoutUnit regionLogicalWidth = region->pageLogicalWidth();
                LayoutUnit regionLogicalHeight = region->pageLogicalHeight();

                if (!firstRegionVisited)
                    firstRegionVisited = true;
                else {
                    if (m_regionsHaveUniformLogicalWidth && previousRegionLogicalWidth != regionLogicalWidth)
                        m_regionsHaveUniformLogicalWidth = false;
                    if (m_regionsHaveUniformLogicalHeight && previousRegionLogicalHeight != regionLogicalHeight)
                        m_regionsHaveUniformLogicalHeight = false;
                }

                previousRegionLogicalWidth = regionLogicalWidth;
            }

            setRegionRangeForBox(*this, m_regionList.first(), m_regionList.last());
        }
    }

    updateLogicalWidth(); // Called to get the maximum logical width for the region.
    updateRegionsFlowThreadPortionRect();
}

void RenderFlowThread::layout()
{
    StackStats::LayoutCheckPoint layoutCheckPoint;

    m_pageLogicalSizeChanged = m_regionsInvalidated && everHadLayout();

    validateRegions();

    RenderBlockFlow::layout();

    m_pageLogicalSizeChanged = false;
}

void RenderFlowThread::updateLogicalWidth()
{
    LayoutUnit logicalWidth = initialLogicalWidth();
    for (auto& region : m_regionList) {
        ASSERT(!region->needsLayout() || region->isRenderRegionSet());
        logicalWidth = std::max(region->pageLogicalWidth(), logicalWidth);
    }
    setLogicalWidth(logicalWidth);

    // If the regions have non-uniform logical widths, then insert inset information for the RenderFlowThread.
    for (auto& region : m_regionList) {
        LayoutUnit regionLogicalWidth = region->pageLogicalWidth();
        LayoutUnit logicalLeft = style().direction() == LTR ? LayoutUnit() : logicalWidth - regionLogicalWidth;
        region->setRenderBoxRegionInfo(this, logicalLeft, regionLogicalWidth, false);
    }
}

RenderBox::LogicalExtentComputedValues RenderFlowThread::computeLogicalHeight(LayoutUnit, LayoutUnit logicalTop) const
{
    LogicalExtentComputedValues computedValues;
    computedValues.m_position = logicalTop;
    computedValues.m_extent = 0;

    const LayoutUnit maxFlowSize = RenderFlowThread::maxLogicalHeight();
    for (auto& region : m_regionList) {
        ASSERT(!region->needsLayout() || region->isRenderRegionSet());

        LayoutUnit distanceToMaxSize = maxFlowSize - computedValues.m_extent;
        computedValues.m_extent += std::min(distanceToMaxSize, region->logicalHeightOfAllFlowThreadContent());

        // If we reached the maximum size there's no point in going further.
        if (computedValues.m_extent == maxFlowSize)
            return computedValues;
    }
    return computedValues;
}

bool RenderFlowThread::nodeAtPoint(const HitTestRequest& request, HitTestResult& result, const HitTestLocation& locationInContainer, const LayoutPoint& accumulatedOffset, HitTestAction hitTestAction)
{
    if (hitTestAction == HitTestBlockBackground)
        return false;
    return RenderBlockFlow::nodeAtPoint(request, result, locationInContainer, accumulatedOffset, hitTestAction);
}

bool RenderFlowThread::shouldRepaint(const LayoutRect& r) const
{
    if (view().printing() || r.isEmpty())
        return false;

    return true;
}

void RenderFlowThread::repaintRectangleInRegions(const LayoutRect& repaintRect) const
{
    if (!shouldRepaint(repaintRect) || !hasValidRegionInfo())
        return;

    LayoutStateDisabler layoutStateDisabler(view()); // We can't use layout state to repaint, since the regions are somewhere else.

    for (auto& region : m_regionList)
        region->repaintFlowThreadContent(repaintRect);
}

RenderRegion* RenderFlowThread::regionAtBlockOffset(const RenderBox* clampBox, LayoutUnit offset, bool extendLastRegion) const
{
    ASSERT(!m_regionsInvalidated);

    if (m_regionList.isEmpty())
        return nullptr;

    if (m_regionList.size() == 1 && extendLastRegion)
        return m_regionList.first();

    if (offset <= 0)
        return clampBox ? clampBox->clampToStartAndEndRegions(m_regionList.first()) : m_regionList.first();

    RegionSearchAdapter adapter(offset);
    m_regionIntervalTree.allOverlapsWithAdapter<RegionSearchAdapter>(adapter);

    // If no region was found, the offset is in the flow thread overflow.
    // The last region will contain the offset if extendLastRegion is set or if the last region is a set.
    if (!adapter.result() && (extendLastRegion || m_regionList.last()->isRenderRegionSet()))
        return clampBox ? clampBox->clampToStartAndEndRegions(m_regionList.last()) : m_regionList.last();

    RenderRegion* region = adapter.result();
    if (!clampBox)
        return region;
    return region ? clampBox->clampToStartAndEndRegions(region) : nullptr;
}

LayoutPoint RenderFlowThread::adjustedPositionRelativeToOffsetParent(const RenderBoxModelObject& boxModelObject, const LayoutPoint& startPoint) const
{
    LayoutPoint referencePoint = startPoint;
    
    const RenderBlock* objContainingBlock = boxModelObject.containingBlock();
    // FIXME: This needs to be adapted for different writing modes inside the flow thread.
    RenderRegion* startRegion = regionAtBlockOffset(objContainingBlock, referencePoint.y());
    if (startRegion) {
        // Take into account the offset coordinates of the region.
        RenderBoxModelObject* startRegionBox = startRegion;
        RenderBoxModelObject* currObject = startRegionBox;
        RenderBoxModelObject* currOffsetParent;
        while ((currOffsetParent = currObject->offsetParent())) {
            referencePoint.move(currObject->offsetLeft(), currObject->offsetTop());
            
            // Since we're looking for the offset relative to the body, we must also
            // take into consideration the borders of the region's offsetParent.
            if (is<RenderBox>(*currOffsetParent) && !currOffsetParent->isBody())
                referencePoint.move(downcast<RenderBox>(*currOffsetParent).borderLeft(), downcast<RenderBox>(*currOffsetParent).borderTop());
            
            currObject = currOffsetParent;
        }
        
        // We need to check if any of this box's containing blocks start in a different region
        // and if so, drop the object's top position (which was computed relative to its containing block
        // and is no longer valid) and recompute it using the region in which it flows as reference.
        bool wasComputedRelativeToOtherRegion = false;
        while (objContainingBlock && !is<RenderView>(*objContainingBlock)) {
            // Check if this object is in a different region.
            RenderRegion* parentStartRegion = nullptr;
            RenderRegion* parentEndRegion = nullptr;
            if (getRegionRangeForBox(objContainingBlock, parentStartRegion, parentEndRegion) && parentStartRegion != startRegion) {
                wasComputedRelativeToOtherRegion = true;
                break;
            }
            objContainingBlock = objContainingBlock->containingBlock();
        }
        
        if (wasComputedRelativeToOtherRegion) {
            if (is<RenderBox>(boxModelObject)) {
                // Use borderBoxRectInRegion to account for variations such as percentage margins.
                LayoutRect borderBoxRect = downcast<RenderBox>(boxModelObject).borderBoxRectInRegion(startRegion, RenderBox::DoNotCacheRenderBoxRegionInfo);
                referencePoint.move(borderBoxRect.location().x(), 0);
            }
            
            // Get the logical top coordinate of the current object.
            LayoutUnit top = 0;
            if (is<RenderBlock>(boxModelObject))
                top = downcast<RenderBlock>(boxModelObject).offsetFromLogicalTopOfFirstPage();
            else {
                if (boxModelObject.containingBlock())
                    top = boxModelObject.containingBlock()->offsetFromLogicalTopOfFirstPage();
                
                if (is<RenderBox>(boxModelObject))
                    top += downcast<RenderBox>(boxModelObject).topLeftLocation().y();
                else if (is<RenderInline>(boxModelObject))
                    top -= downcast<RenderInline>(boxModelObject).borderTop();
            }
            
            // Get the logical top of the region this object starts in
            // and compute the object's top, relative to the region's top.
            LayoutUnit regionLogicalTop = startRegion->pageLogicalTopForOffset(top);
            LayoutUnit topRelativeToRegion = top - regionLogicalTop;
            referencePoint.setY(startRegionBox->offsetTop() + topRelativeToRegion);
            
            // Since the top has been overriden, check if the
            // relative/sticky positioning must be reconsidered.
            if (boxModelObject.isRelPositioned())
                referencePoint.move(0, boxModelObject.relativePositionOffset().height());
            else if (boxModelObject.isStickyPositioned())
                referencePoint.move(0, boxModelObject.stickyPositionOffset().height());
        }
        
        // Since we're looking for the offset relative to the body, we must also
        // take into consideration the borders of the region.
        referencePoint.move(startRegionBox->borderLeft(), startRegionBox->borderTop());
    }
    
    return referencePoint;
}

LayoutUnit RenderFlowThread::pageLogicalTopForOffset(LayoutUnit offset) const
{
    RenderRegion* region = regionAtBlockOffset(0, offset, false);
    return region ? region->pageLogicalTopForOffset(offset) : LayoutUnit();
}

LayoutUnit RenderFlowThread::pageLogicalWidthForOffset(LayoutUnit offset) const
{
    RenderRegion* region = regionAtBlockOffset(0, offset, true);
    return region ? region->pageLogicalWidth() : contentLogicalWidth();
}

LayoutUnit RenderFlowThread::pageLogicalHeightForOffset(LayoutUnit offset) const
{
    RenderRegion* region = regionAtBlockOffset(0, offset, false);
    if (!region)
        return 0;

    return region->pageLogicalHeight();
}

LayoutUnit RenderFlowThread::pageRemainingLogicalHeightForOffset(LayoutUnit offset, PageBoundaryRule pageBoundaryRule) const
{
    RenderRegion* region = regionAtBlockOffset(0, offset, false);
    if (!region)
        return 0;

    LayoutUnit pageLogicalTop = region->pageLogicalTopForOffset(offset);
    LayoutUnit pageLogicalHeight = region->pageLogicalHeight();
    LayoutUnit pageLogicalBottom = pageLogicalTop + pageLogicalHeight;
    LayoutUnit remainingHeight = pageLogicalBottom - offset;
    if (pageBoundaryRule == IncludePageBoundary) {
        // If IncludePageBoundary is set, the line exactly on the top edge of a
        // region will act as being part of the previous region.
        remainingHeight = intMod(remainingHeight, pageLogicalHeight);
    }
    return remainingHeight;
}

RenderRegion* RenderFlowThread::mapFromFlowToRegion(TransformState& transformState) const
{
    if (!hasValidRegionInfo())
        return nullptr;

    RenderRegion* renderRegion = currentRegion();
    if (!renderRegion) {
        LayoutRect boxRect = transformState.mappedQuad().enclosingBoundingBox();
        flipForWritingMode(boxRect);

        LayoutPoint center = boxRect.center();
        renderRegion = regionAtBlockOffset(this, isHorizontalWritingMode() ? center.y() : center.x(), true);
        if (!renderRegion)
            return nullptr;
    }

    LayoutRect flippedRegionRect(renderRegion->flowThreadPortionRect());
    flipForWritingMode(flippedRegionRect);

    transformState.move(renderRegion->contentBoxRect().location() - flippedRegionRect.location());

    return renderRegion;
}

void RenderFlowThread::removeRenderBoxRegionInfo(RenderBox& box)
{
    if (!hasRegions())
        return;

    // If the region chain was invalidated the next layout will clear the box information from all the regions.
    if (m_regionsInvalidated) {
        ASSERT(selfNeedsLayout());
        return;
    }

    RenderRegion* startRegion = nullptr;
    RenderRegion* endRegion = nullptr;
    if (getRegionRangeForBox(&box, startRegion, endRegion)) {
        for (auto it = m_regionList.find(startRegion), end = m_regionList.end(); it != end; ++it) {
            RenderRegion* region = *it;
            region->removeRenderBoxRegionInfo(box);
            if (region == endRegion)
                break;
        }
    }

#ifndef NDEBUG
    // We have to make sure we did not leave any RenderBoxRegionInfo attached.
    for (auto& region : m_regionList)
        ASSERT(!region->renderBoxRegionInfo(&box));
#endif

    m_regionRangeMap.remove(&box);
}

void RenderFlowThread::removeLineRegionInfo(const RenderBlockFlow& blockFlow)
{
    if (!m_lineToRegionMap || blockFlow.lineLayoutPath() == SimpleLinesPath)
        return;

    for (auto* curr = blockFlow.firstRootBox(); curr; curr = curr->nextRootBox())
        m_lineToRegionMap->remove(curr);

    ASSERT_WITH_SECURITY_IMPLICATION(checkLinesConsistency(blockFlow));
}

void RenderFlowThread::logicalWidthChangedInRegionsForBlock(const RenderBlock* block, bool& relayoutChildren)
{
    if (!hasValidRegionInfo())
        return;

    auto it = m_regionRangeMap.find(block);
    if (it == m_regionRangeMap.end())
        return;

    RenderRegionRange& range = it->value;
    bool rangeInvalidated = range.rangeInvalidated();
    range.clearRangeInvalidated();

    // If there will be a relayout anyway skip the next steps because they only verify
    // the state of the ranges.
    if (relayoutChildren)
        return;

    // Not necessary for the flow thread, since we already computed the correct info for it.
    // If the regions have changed invalidate the children.
    if (block == this) {
        relayoutChildren = m_pageLogicalSizeChanged;
        return;
    }

    RenderRegion* startRegion = nullptr;
    RenderRegion* endRegion = nullptr;
    if (!getRegionRangeForBox(block, startRegion, endRegion))
        return;

    for (auto it = m_regionList.find(startRegion), end = m_regionList.end(); it != end; ++it) {
        RenderRegion* region = *it;
        ASSERT(!region->needsLayout() || region->isRenderRegionSet());

        // We have no information computed for this region so we need to do it.
        std::unique_ptr<RenderBoxRegionInfo> oldInfo = region->takeRenderBoxRegionInfo(block);
        if (!oldInfo) {
            relayoutChildren = rangeInvalidated;
            return;
        }

        LayoutUnit oldLogicalWidth = oldInfo->logicalWidth();
        RenderBoxRegionInfo* newInfo = block->renderBoxRegionInfo(region);
        if (!newInfo || newInfo->logicalWidth() != oldLogicalWidth) {
            relayoutChildren = true;
            return;
        }

        if (region == endRegion)
            break;
    }
}

LayoutUnit RenderFlowThread::contentLogicalWidthOfFirstRegion() const
{
    RenderRegion* firstValidRegionInFlow = firstRegion();
    if (!firstValidRegionInFlow)
        return 0;
    return isHorizontalWritingMode() ? firstValidRegionInFlow->contentWidth() : firstValidRegionInFlow->contentHeight();
}

LayoutUnit RenderFlowThread::contentLogicalHeightOfFirstRegion() const
{
    RenderRegion* firstValidRegionInFlow = firstRegion();
    if (!firstValidRegionInFlow)
        return 0;
    return isHorizontalWritingMode() ? firstValidRegionInFlow->contentHeight() : firstValidRegionInFlow->contentWidth();
}

LayoutUnit RenderFlowThread::contentLogicalLeftOfFirstRegion() const
{
    RenderRegion* firstValidRegionInFlow = firstRegion();
    if (!firstValidRegionInFlow)
        return 0;
    return isHorizontalWritingMode() ? firstValidRegionInFlow->flowThreadPortionRect().x() : firstValidRegionInFlow->flowThreadPortionRect().y();
}

RenderRegion* RenderFlowThread::firstRegion() const
{
    if (!hasRegions())
        return nullptr;
    return m_regionList.first();
}

RenderRegion* RenderFlowThread::lastRegion() const
{
    if (!hasRegions())
        return nullptr;
    return m_regionList.last();
}

void RenderFlowThread::clearRenderBoxRegionInfoAndCustomStyle(const RenderBox& box,
    const RenderRegion* newStartRegion, const RenderRegion* newEndRegion,
    const RenderRegion* oldStartRegion, const RenderRegion* oldEndRegion)
{
    ASSERT(newStartRegion && newEndRegion && oldStartRegion && oldEndRegion);

    bool insideOldRegionRange = false;
    bool insideNewRegionRange = false;
    for (auto& region : m_regionList) {
        if (oldStartRegion == region)
            insideOldRegionRange = true;
        if (newStartRegion == region)
            insideNewRegionRange = true;

        if (!(insideOldRegionRange && insideNewRegionRange)) {
            if (region->renderBoxRegionInfo(&box))
                region->removeRenderBoxRegionInfo(box);
        }

        if (oldEndRegion == region)
            insideOldRegionRange = false;
        if (newEndRegion == region)
            insideNewRegionRange = false;
    }
}

void RenderFlowThread::setRegionRangeForBox(const RenderBox& box, RenderRegion* startRegion, RenderRegion* endRegion)
{
    ASSERT(hasRegions());
    ASSERT(startRegion && endRegion && startRegion->flowThread() == this && endRegion->flowThread() == this);

    auto it = m_regionRangeMap.find(&box);
    if (it == m_regionRangeMap.end()) {
        m_regionRangeMap.set(&box, RenderRegionRange(startRegion, endRegion));
        return;
    }

    // If nothing changed, just bail.
    RenderRegionRange& range = it->value;
    if (range.startRegion() == startRegion && range.endRegion() == endRegion)
        return;

    clearRenderBoxRegionInfoAndCustomStyle(box, startRegion, endRegion, range.startRegion(), range.endRegion());
    range.setRange(startRegion, endRegion);
}

bool RenderFlowThread::hasCachedRegionRangeForBox(const RenderBox& box) const
{
    return m_regionRangeMap.contains(&box);
}

bool RenderFlowThread::getRegionRangeForBoxFromCachedInfo(const RenderBox* box, RenderRegion*& startRegion, RenderRegion*& endRegion) const
{
    ASSERT(box);
    ASSERT(hasValidRegionInfo());
    ASSERT((startRegion == nullptr) && (endRegion == nullptr));

    auto it = m_regionRangeMap.find(box);
    if (it != m_regionRangeMap.end()) {
        const RenderRegionRange& range = it->value;
        startRegion = range.startRegion();
        endRegion = range.endRegion();
        ASSERT(m_regionList.contains(startRegion) && m_regionList.contains(endRegion));
        return true;
    }

    return false;
}

bool RenderFlowThread::getRegionRangeForBox(const RenderBox* box, RenderRegion*& startRegion, RenderRegion*& endRegion) const
{
    ASSERT(box);

    startRegion = endRegion = nullptr;
    if (!hasValidRegionInfo()) // We clear the ranges when we invalidate the regions.
        return false;

    if (m_regionList.size() == 1) {
        startRegion = endRegion = m_regionList.first();
        return true;
    }

    if (getRegionRangeForBoxFromCachedInfo(box, startRegion, endRegion))
        return true;

    return false;
}

bool RenderFlowThread::computedRegionRangeForBox(const RenderBox* box, RenderRegion*& startRegion, RenderRegion*& endRegion) const
{
    ASSERT(box);

    startRegion = endRegion = nullptr;
    if (!hasValidRegionInfo()) // We clear the ranges when we invalidate the regions.
        return false;

    if (getRegionRangeForBox(box, startRegion, endRegion))
        return true;

    // Search the region range using the information provided by the containing block chain.
    auto* containingBlock = const_cast<RenderBox*>(box);
    while (!containingBlock->isRenderFlowThread()) {
        InlineElementBox* boxWrapper = containingBlock->inlineBoxWrapper();
        if (boxWrapper && boxWrapper->root().containingRegion()) {
            startRegion = endRegion = boxWrapper->root().containingRegion();
            ASSERT(m_regionList.contains(startRegion));
            return true;
        }

        // FIXME: Use the containingBlock() value once we patch all the layout systems to be region range aware
        // (e.g. if we use containingBlock() the shadow controls of a video element won't get the range from the
        // video box because it's not a block; they need to be patched separately).
        ASSERT(containingBlock->parent());
        containingBlock = &containingBlock->parent()->enclosingBox();
        ASSERT(containingBlock);

        // If a box doesn't have a cached region range it usually means the box belongs to a line so startRegion should be equal with endRegion.
        // FIXME: Find the cases when this startRegion should not be equal with endRegion and make sure these boxes have cached region ranges.
        if (containingBlock && hasCachedRegionRangeForBox(*containingBlock)) {
            startRegion = endRegion = regionAtBlockOffset(containingBlock, containingBlock->offsetFromLogicalTopOfFirstPage(), true);
            return true;
        }
    }
    ASSERT_NOT_REACHED();
    return false;
}

bool RenderFlowThread::regionInRange(const RenderRegion* targetRegion, const RenderRegion* startRegion, const RenderRegion* endRegion) const
{
    ASSERT(targetRegion);

    for (auto it = m_regionList.find(const_cast<RenderRegion*>(startRegion)), end = m_regionList.end(); it != end; ++it) {
        const RenderRegion* currRegion = *it;
        if (targetRegion == currRegion)
            return true;
        if (currRegion == endRegion)
            break;
    }

    return false;
}

bool RenderFlowThread::objectShouldFragmentInFlowRegion(const RenderObject* object, const RenderRegion* region) const
{
    ASSERT(object);
    ASSERT(region);
    
    RenderFlowThread* flowThread = object->flowThreadContainingBlock();
    if (flowThread != this)
        return false;

    if (!m_regionList.contains(const_cast<RenderRegion*>(region)))
        return false;
    
    RenderRegion* enclosingBoxStartRegion = nullptr;
    RenderRegion* enclosingBoxEndRegion = nullptr;
    // If the box has no range, do not check regionInRange. Boxes inside inlines do not get ranges.
    // Instead, the containing RootInlineBox will abort when trying to paint inside the wrong region.
    if (computedRegionRangeForBox(&object->enclosingBox(), enclosingBoxStartRegion, enclosingBoxEndRegion)
        && !regionInRange(region, enclosingBoxStartRegion, enclosingBoxEndRegion))
        return false;
    
    return object->isBox() || object->isRenderInline();
}

bool RenderFlowThread::objectInFlowRegion(const RenderObject* object, const RenderRegion* region) const
{
    ASSERT(object);
    ASSERT(region);

    RenderFlowThread* flowThread = object->flowThreadContainingBlock();
    if (flowThread != this)
        return false;

    if (!m_regionList.contains(const_cast<RenderRegion*>(region)))
        return false;

    RenderRegion* enclosingBoxStartRegion = nullptr;
    RenderRegion* enclosingBoxEndRegion = nullptr;
    if (!getRegionRangeForBox(&object->enclosingBox(), enclosingBoxStartRegion, enclosingBoxEndRegion))
        return false;

    if (!regionInRange(region, enclosingBoxStartRegion, enclosingBoxEndRegion))
        return false;

    if (object->isBox())
        return true;

    LayoutRect objectABBRect = object->absoluteBoundingBoxRect(true);
    if (!objectABBRect.width())
        objectABBRect.setWidth(1);
    if (!objectABBRect.height())
        objectABBRect.setHeight(1); 
    if (objectABBRect.intersects(region->absoluteBoundingBoxRect(true)))
        return true;

    if (region == lastRegion()) {
        // If the object does not intersect any of the enclosing box regions
        // then the object is in last region.
        for (auto it = m_regionList.find(enclosingBoxStartRegion), end = m_regionList.end(); it != end; ++it) {
            const RenderRegion* currRegion = *it;
            if (currRegion == region)
                break;
            if (objectABBRect.intersects(currRegion->absoluteBoundingBoxRect(true)))
                return false;
        }
        return true;
    }

    return false;
}

#if !ASSERT_WITH_SECURITY_IMPLICATION_DISABLED
bool RenderFlowThread::checkLinesConsistency(const RenderBlockFlow& removedBlock) const
{
    if (!m_lineToRegionMap)
        return true;

    for (auto& linePair : *m_lineToRegionMap.get()) {
        const RootInlineBox* line = linePair.key;
        RenderRegion* region = linePair.value;
        if (&line->blockFlow() == &removedBlock)
            return false;
        if (line->blockFlow().flowThreadState() == NotInsideFlowThread)
            return false;
        if (!m_regionList.contains(region))
            return false;
    }

    return true;
}
#endif

void RenderFlowThread::clearLinesToRegionMap()
{
    if (m_lineToRegionMap)
        m_lineToRegionMap->clear();
}

void RenderFlowThread::deleteLines()
{
    clearLinesToRegionMap();
    RenderBlockFlow::deleteLines();
}

void RenderFlowThread::willBeDestroyed()
{
    clearLinesToRegionMap();
    RenderBlockFlow::willBeDestroyed();
}

void RenderFlowThread::markRegionsForOverflowLayoutIfNeeded()
{
    if (!hasRegions())
        return;

    for (auto& region : m_regionList)
        region->setNeedsSimplifiedNormalFlowLayout();
}

void RenderFlowThread::updateRegionsFlowThreadPortionRect()
{
    LayoutUnit logicalHeight = 0;
    // FIXME: Optimize not to clear the interval all the time. This implies manually managing the tree nodes lifecycle.
    m_regionIntervalTree.clear();
    for (auto& region : m_regionList) {
        LayoutUnit regionLogicalWidth = region->pageLogicalWidth();
        LayoutUnit regionLogicalHeight = std::min<LayoutUnit>(RenderFlowThread::maxLogicalHeight() - logicalHeight, region->logicalHeightOfAllFlowThreadContent());

        LayoutRect regionRect(style().direction() == LTR ? LayoutUnit() : logicalWidth() - regionLogicalWidth, logicalHeight, regionLogicalWidth, regionLogicalHeight);

        region->setFlowThreadPortionRect(isHorizontalWritingMode() ? regionRect : regionRect.transposedRect());

        m_regionIntervalTree.add(RegionIntervalTree::createInterval(logicalHeight, logicalHeight + regionLogicalHeight, region));

        logicalHeight += regionLogicalHeight;
    }
}

// Even if we require the break to occur at offsetBreakInFlowThread, because regions may have min/max-height values,
// it is possible that the break will occur at a different offset than the original one required.
// offsetBreakAdjustment measures the different between the requested break offset and the current break offset.
bool RenderFlowThread::addForcedRegionBreak(const RenderBlock* block, LayoutUnit offsetBreakInFlowThread, RenderBox*, bool, LayoutUnit* offsetBreakAdjustment)
{
    // We need to update the regions flow thread portion rect because we are going to process
    // a break on these regions.
    updateRegionsFlowThreadPortionRect();

    // Simulate a region break at offsetBreakInFlowThread. If it points inside an auto logical height region,
    // then it determines the region computed auto height.
    RenderRegion* region = regionAtBlockOffset(block, offsetBreakInFlowThread);
    if (!region)
        return false;

    LayoutUnit currentRegionOffsetInFlowThread = isHorizontalWritingMode() ? region->flowThreadPortionRect().y() : region->flowThreadPortionRect().x();

    currentRegionOffsetInFlowThread += isHorizontalWritingMode() ? region->flowThreadPortionRect().height() : region->flowThreadPortionRect().width();

    if (offsetBreakAdjustment)
        *offsetBreakAdjustment = std::max<LayoutUnit>(0, currentRegionOffsetInFlowThread - offsetBreakInFlowThread);

    return false;
}

void RenderFlowThread::collectLayerFragments(LayerFragments& layerFragments, const LayoutRect& layerBoundingBox, const LayoutRect& dirtyRect)
{
    ASSERT(!m_regionsInvalidated);
    
    for (auto& region : m_regionList)
        region->collectLayerFragments(layerFragments, layerBoundingBox, dirtyRect);
}

LayoutRect RenderFlowThread::fragmentsBoundingBox(const LayoutRect& layerBoundingBox)
{
    ASSERT(!m_regionsInvalidated);
    
    LayoutRect result;
    for (auto& region : m_regionList) {
        LayerFragments fragments;
        region->collectLayerFragments(fragments, layerBoundingBox, LayoutRect::infiniteRect());
        for (const auto& fragment : fragments) {
            LayoutRect fragmentRect(layerBoundingBox);
            fragmentRect.intersect(fragment.paginationClip);
            fragmentRect.move(fragment.paginationOffset);
            result.unite(fragmentRect);
        }
    }
    
    return result;
}

LayoutUnit RenderFlowThread::offsetFromLogicalTopOfFirstRegion(const RenderBlock* currentBlock) const
{
    // As a last resort, take the slow path.
    LayoutRect blockRect(0, 0, currentBlock->width(), currentBlock->height());
    while (currentBlock && !is<RenderView>(*currentBlock) && !currentBlock->isRenderFlowThread()) {
        RenderBlock* containerBlock = currentBlock->containingBlock();
        ASSERT(containerBlock);
        if (!containerBlock)
            return 0;
        LayoutPoint currentBlockLocation = currentBlock->location();
        if (is<RenderTableCell>(*currentBlock)) {
            if (auto* section = downcast<RenderTableCell>(*currentBlock).section())
                currentBlockLocation.moveBy(section->location());
        }

        if (containerBlock->style().writingMode() != currentBlock->style().writingMode()) {
            // We have to put the block rect in container coordinates
            // and we have to take into account both the container and current block flipping modes
            if (containerBlock->style().isFlippedBlocksWritingMode()) {
                if (containerBlock->isHorizontalWritingMode())
                    blockRect.setY(currentBlock->height() - blockRect.maxY());
                else
                    blockRect.setX(currentBlock->width() - blockRect.maxX());
            }
            currentBlock->flipForWritingMode(blockRect);
        }
        blockRect.moveBy(currentBlockLocation);
        currentBlock = containerBlock;
    }

    return currentBlock->isHorizontalWritingMode() ? blockRect.y() : blockRect.x();
}

void RenderFlowThread::RegionSearchAdapter::collectIfNeeded(const RegionInterval& interval)
{
    if (m_result)
        return;
    if (interval.low() <= m_offset && interval.high() > m_offset)
        m_result = interval.data();
}

void RenderFlowThread::mapLocalToContainer(const RenderLayerModelObject* repaintContainer, TransformState& transformState, MapCoordinatesFlags mode, bool* wasFixed) const
{
    if (this == repaintContainer)
        return;

    if (RenderRegion* region = mapFromFlowToRegion(transformState)) {
        // FIXME: The cast below is probably not the best solution, we may need to find a better way.
        const RenderObject* regionObject = static_cast<const RenderObject*>(region);

        // If the repaint container is nullptr, we have to climb up to the RenderView, otherwise swap
        // it with the region's repaint container.
        repaintContainer = repaintContainer ? region->containerForRepaint() : nullptr;

        if (RenderFlowThread* regionFlowThread = region->flowThreadContainingBlock()) {
            RenderRegion* startRegion = nullptr;
            RenderRegion* endRegion = nullptr;
            if (regionFlowThread->getRegionRangeForBox(region, startRegion, endRegion)) {
                CurrentRenderRegionMaintainer regionMaintainer(*startRegion);
                regionObject->mapLocalToContainer(repaintContainer, transformState, mode, wasFixed);
                return;
            }
        }

        regionObject->mapLocalToContainer(repaintContainer, transformState, mode, wasFixed);
    }
}

// FIXME: Make this function faster. Walking the render tree is slow, better use a caching mechanism (e.g. |cachedOffsetFromLogicalTopOfFirstRegion|).
LayoutRect RenderFlowThread::mapFromLocalToFlowThread(const RenderBox* box, const LayoutRect& localRect) const
{
    LayoutRect boxRect = localRect;

    while (box && box != this) {
        RenderBlock* containerBlock = box->containingBlock();
        ASSERT(containerBlock);
        if (!containerBlock)
            return LayoutRect();
        LayoutPoint currentBoxLocation = box->location();

        if (containerBlock->style().writingMode() != box->style().writingMode())
            box->flipForWritingMode(boxRect);

        boxRect.moveBy(currentBoxLocation);
        box = containerBlock;
    }

    return boxRect;
}

// FIXME: Make this function faster. Walking the render tree is slow, better use a caching mechanism (e.g. |cachedOffsetFromLogicalTopOfFirstRegion|).
LayoutRect RenderFlowThread::mapFromFlowThreadToLocal(const RenderBox* box, const LayoutRect& rect) const
{
    LayoutRect localRect = rect;
    if (box == this)
        return localRect;

    RenderBlock* containerBlock = box->containingBlock();
    ASSERT(containerBlock);
    if (!containerBlock)
        return LayoutRect();
    localRect = mapFromFlowThreadToLocal(containerBlock, localRect);

    LayoutPoint currentBoxLocation = box->location();
    localRect.moveBy(-currentBoxLocation);

    if (containerBlock->style().writingMode() != box->style().writingMode())
        box->flipForWritingMode(localRect);

    return localRect;
}

void RenderFlowThread::flipForWritingModeLocalCoordinates(LayoutRect& rect) const
{
    if (!style().isFlippedBlocksWritingMode())
        return;
    
    if (isHorizontalWritingMode())
        rect.setY(0 - rect.maxY());
    else
        rect.setX(0 - rect.maxX());
}

void RenderFlowThread::addRegionsVisualEffectOverflow(const RenderBox* box)
{
    RenderRegion* startRegion = nullptr;
    RenderRegion* endRegion = nullptr;
    if (!getRegionRangeForBox(box, startRegion, endRegion))
        return;

    for (auto iter = m_regionList.find(startRegion), end = m_regionList.end(); iter != end; ++iter) {
        RenderRegion* region = *iter;

        LayoutRect borderBox = box->borderBoxRectInRegion(region);
        borderBox = box->applyVisualEffectOverflow(borderBox);
        borderBox = region->rectFlowPortionForBox(box, borderBox);

        region->addVisualOverflowForBox(box, borderBox);
        if (region == endRegion)
            break;
    }
}

void RenderFlowThread::addRegionsVisualOverflowFromTheme(const RenderBlock* block)
{
    RenderRegion* startRegion = nullptr;
    RenderRegion* endRegion = nullptr;
    if (!getRegionRangeForBox(block, startRegion, endRegion))
        return;

    for (auto iter = m_regionList.find(startRegion), end = m_regionList.end(); iter != end; ++iter) {
        RenderRegion* region = *iter;

        LayoutRect borderBox = block->borderBoxRectInRegion(region);
        borderBox = region->rectFlowPortionForBox(block, borderBox);

        FloatRect inflatedRect = borderBox;
        block->theme().adjustRepaintRect(*block, inflatedRect);

        region->addVisualOverflowForBox(block, snappedIntRect(LayoutRect(inflatedRect)));
        if (region == endRegion)
            break;
    }
}

void RenderFlowThread::addRegionsOverflowFromChild(const RenderBox* box, const RenderBox* child, const LayoutSize& delta)
{
    RenderRegion* startRegion = nullptr;
    RenderRegion* endRegion = nullptr;
    if (!getRegionRangeForBox(child, startRegion, endRegion))
        return;

    RenderRegion* containerStartRegion = nullptr;
    RenderRegion* containerEndRegion = nullptr;
    if (!getRegionRangeForBox(box, containerStartRegion, containerEndRegion))
        return;

    for (auto iter = m_regionList.find(startRegion), end = m_regionList.end(); iter != end; ++iter) {
        RenderRegion* region = *iter;
        if (!regionInRange(region, containerStartRegion, containerEndRegion)) {
            if (region == endRegion)
                break;
            continue;
        }

        LayoutRect childLayoutOverflowRect = region->layoutOverflowRectForBoxForPropagation(child);
        childLayoutOverflowRect.move(delta);
        
        region->addLayoutOverflowForBox(box, childLayoutOverflowRect);

        if (child->hasSelfPaintingLayer() || box->hasOverflowClip()) {
            if (region == endRegion)
                break;
            continue;
        }
        LayoutRect childVisualOverflowRect = region->visualOverflowRectForBoxForPropagation(*child);
        childVisualOverflowRect.move(delta);
        region->addVisualOverflowForBox(box, childVisualOverflowRect);

        if (region == endRegion)
            break;
    }
}
    
void RenderFlowThread::addRegionsLayoutOverflow(const RenderBox* box, const LayoutRect& layoutOverflow)
{
    RenderRegion* startRegion = nullptr;
    RenderRegion* endRegion = nullptr;
    if (!getRegionRangeForBox(box, startRegion, endRegion))
        return;

    for (auto iter = m_regionList.find(startRegion), end = m_regionList.end(); iter != end; ++iter) {
        RenderRegion* region = *iter;
        LayoutRect layoutOverflowInRegion = region->rectFlowPortionForBox(box, layoutOverflow);

        region->addLayoutOverflowForBox(box, layoutOverflowInRegion);

        if (region == endRegion)
            break;
    }
}

void RenderFlowThread::addRegionsVisualOverflow(const RenderBox* box, const LayoutRect& visualOverflow)
{
    RenderRegion* startRegion = nullptr;
    RenderRegion* endRegion = nullptr;
    if (!getRegionRangeForBox(box, startRegion, endRegion))
        return;
    
    for (RenderRegionList::iterator iter = m_regionList.find(startRegion); iter != m_regionList.end(); ++iter) {
        RenderRegion* region = *iter;
        LayoutRect visualOverflowInRegion = region->rectFlowPortionForBox(box, visualOverflow);
        
        region->addVisualOverflowForBox(box, visualOverflowInRegion);
        
        if (region == endRegion)
            break;
    }
}

void RenderFlowThread::clearRegionsOverflow(const RenderBox* box)
{
    RenderRegion* startRegion = nullptr;
    RenderRegion* endRegion = nullptr;
    if (!getRegionRangeForBox(box, startRegion, endRegion))
        return;

    for (auto iter = m_regionList.find(startRegion), end = m_regionList.end(); iter != end; ++iter) {
        RenderRegion* region = *iter;
        RenderBoxRegionInfo* boxInfo = region->renderBoxRegionInfo(box);
        if (boxInfo && boxInfo->overflow())
            boxInfo->clearOverflow();

        if (region == endRegion)
            break;
    }
}

RenderRegion* RenderFlowThread::currentRegion() const
{
    return m_currentRegionMaintainer ? &m_currentRegionMaintainer->region() : nullptr;
}

ContainingRegionMap& RenderFlowThread::containingRegionMap()
{
    if (!m_lineToRegionMap)
        m_lineToRegionMap = std::make_unique<ContainingRegionMap>();

    return *m_lineToRegionMap.get();
}


} // namespace WebCore
