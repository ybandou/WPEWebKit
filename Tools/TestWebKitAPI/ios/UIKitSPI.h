/*
 * Copyright (C) 2017 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#if PLATFORM(IOS)

#import <UIKit/UITextInputTraits.h>

#if __IPHONE_OS_VERSION_MAX_ALLOWED >= 110000
@protocol UIDragSession;
@class UIDragInteraction;
@class UIDragItem;
#endif

#if USE(APPLE_INTERNAL_SDK) && __IPHONE_OS_VERSION_MAX_ALLOWED >= 110000

#import <UIKit/UIApplication_Private.h>
#import <UIKit/UIDragInteraction_Private.h>
#import <UIKit/UITextInputTraits_Private.h>
#import <UIKit/UITextSuggestion.h>


@protocol UITextInputTraits_Private_Staging_34583628 <NSObject, UITextInputTraits>
@property (nonatomic, readonly) BOOL acceptsAutofilledLoginCredentials;
@end

#else

WTF_EXTERN_C_BEGIN

void UIApplicationInitialize(void);

WTF_EXTERN_C_END

#if __IPHONE_OS_VERSION_MAX_ALLOWED >= 110000
@protocol UIDragInteractionDelegate_ForWebKitOnly <UIDragInteractionDelegate>
@optional
- (void)_dragInteraction:(UIDragInteraction *)interaction prepareForSession:(id<UIDragSession>)session completion:(void(^)(void))completion;
@end
#endif // __IPHONE_OS_VERSION_MAX_ALLOWED >= 110000

@protocol UITextInputTraits_Private <NSObject, UITextInputTraits>
@property (nonatomic, readonly) BOOL acceptsAutofilledLoginCredentials;
@end

#endif

#if __IPHONE_OS_VERSION_MAX_ALLOWED >= 110000
@protocol UIDragInteractionDelegate_Proposed_SPI_33146803 <UIDragInteractionDelegate>
- (void)_dragInteraction:(UIDragInteraction *)interaction itemsForAddingToSession:(id <UIDragSession>)session withTouchAtPoint:(CGPoint)point completion:(void(^)(NSArray<UIDragItem *> *))completion;
@end
#endif // __IPHONE_OS_VERSION_MAX_ALLOWED >= 110000

#endif // PLATFORM(IOS)
