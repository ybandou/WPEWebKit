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

#include "config.h"
#include "PaymentRequest.h"

#if ENABLE(PAYMENT_REQUEST)

#include "Document.h"
#include "PaymentAddress.h"
#include "PaymentCurrencyAmount.h"
#include "PaymentDetailsInit.h"
#include "PaymentMethodData.h"
#include "PaymentOptions.h"
#include "ScriptController.h"
#include <JavaScriptCore/JSONObject.h>
#include <JavaScriptCore/ThrowScope.h>
#include <wtf/ASCIICType.h>
#include <wtf/RunLoop.h>
#include <wtf/UUID.h>

namespace WebCore {

// Implements the IsWellFormedCurrencyCode abstract operation from ECMA 402
// https://tc39.github.io/ecma402/#sec-iswellformedcurrencycode
static bool isWellFormedCurrencyCode(const String& currency)
{
    if (currency.length() == 3)
        return currency.isAllSpecialCharacters<isASCIIAlpha>();
    return false;
}

// Implements the "valid decimal monetary value" validity checker
// https://www.w3.org/TR/payment-request/#dfn-valid-decimal-monetary-value
static bool isValidDecimalMonetaryValue(StringView value)
{
    enum class State {
        Start,
        Sign,
        Digit,
        Dot,
        DotDigit,
    };

    auto state = State::Start;
    for (auto character : value.codeUnits()) {
        switch (state) {
        case State::Start:
            if (character == '-') {
                state = State::Sign;
                break;
            }

            if (isASCIIDigit(character)) {
                state = State::Digit;
                break;
            }

            return false;

        case State::Sign:
            if (isASCIIDigit(character)) {
                state = State::Digit;
                break;
            }

            return false;

        case State::Digit:
            if (character == '.') {
                state = State::Dot;
                break;
            }

            if (isASCIIDigit(character)) {
                state = State::Digit;
                break;
            }

            return false;

        case State::Dot:
            if (isASCIIDigit(character)) {
                state = State::DotDigit;
                break;
            }

            return false;

        case State::DotDigit:
            if (isASCIIDigit(character)) {
                state = State::DotDigit;
                break;
            }

            return false;
        }
    }

    if (state == State::Digit || state == State::DotDigit)
        return true;

    return false;
}

// Implements the "check and canonicalize amount" validity checker
// https://www.w3.org/TR/payment-request/#dfn-check-and-canonicalize-amount
static ExceptionOr<void> checkAndCanonicalizeAmount(PaymentCurrencyAmount& amount)
{
    if (amount.currencySystem != "urn:iso:std:iso:4217")
        return { };

    if (!isWellFormedCurrencyCode(amount.currency))
        return Exception { RangeError, makeString("\"", amount.currency, "\" is not a valid currency code.") };

    if (!isValidDecimalMonetaryValue(amount.value))
        return Exception { TypeError, makeString("\"", amount.value, "\" is not a valid decimal monetary value.") };

    amount.currency = amount.currency.convertToASCIIUppercase();
    return { };
}

// Implements the "check and canonicalize total" validity checker
// https://www.w3.org/TR/payment-request/#dfn-check-and-canonicalize-total
static ExceptionOr<void> checkAndCanonicalizeTotal(PaymentCurrencyAmount& total)
{
    if (total.currencySystem != "urn:iso:std:iso:4217")
        return { };

    auto exception = checkAndCanonicalizeAmount(total);
    if (exception.hasException())
        return exception;

    if (total.value[0] == '-')
        return Exception { TypeError, ASCIILiteral("Total currency values cannot be negative.") };

    return { };
}

// Implements the PaymentRequest Constructor
// https://www.w3.org/TR/payment-request/#constructor
ExceptionOr<Ref<PaymentRequest>> PaymentRequest::create(Document& document, Vector<PaymentMethodData>&& methodData, PaymentDetailsInit&& details, PaymentOptions&& options)
{
    // FIXME: Check if this document is allowed to access the PaymentRequest API based on the allowpaymentrequest attribute.

    if (details.id.isNull())
        details.id = createCanonicalUUIDString();

    if (methodData.isEmpty())
        return Exception { TypeError, ASCIILiteral("At least one payment method is required.") };

    Vector<Method> serializedMethodData;
    serializedMethodData.reserveInitialCapacity(methodData.size());
    for (auto& paymentMethod : methodData) {
        if (paymentMethod.supportedMethods.isEmpty())
            return Exception { TypeError, ASCIILiteral("supportedMethods must be specified.") };

        String serializedData;
        if (paymentMethod.data) {
            auto scope = DECLARE_THROW_SCOPE(document.execState()->vm());
            serializedData = JSONStringify(document.execState(), paymentMethod.data.get(), 0);
            if (scope.exception())
                return Exception { ExistingExceptionError };
        }
        serializedMethodData.uncheckedAppend({ paymentMethod.supportedMethods, WTFMove(serializedData) });
    }

    auto exception = checkAndCanonicalizeTotal(details.total.amount);
    if (exception.hasException())
        return exception.releaseException();

    for (auto& item : details.displayItems) {
        auto exception = checkAndCanonicalizeAmount(item.amount);
        if (exception.hasException())
            return exception.releaseException();
    }

    String selectedShippingOption;
    HashSet<String> seenShippingOptionIDs;
    for (auto& shippingOption : details.shippingOptions) {
        auto exception = checkAndCanonicalizeAmount(shippingOption.amount);
        if (exception.hasException())
            return exception.releaseException();

        auto addResult = seenShippingOptionIDs.add(shippingOption.id);
        if (!addResult.isNewEntry) {
            details.shippingOptions = { };
            selectedShippingOption = { };
            break;
        }

        if (shippingOption.selected)
            selectedShippingOption = shippingOption.id;
    }

    Vector<String> serializedModifierData;
    serializedModifierData.reserveInitialCapacity(details.modifiers.size());
    for (auto& modifier : details.modifiers) {
        if (modifier.total) {
            auto exception = checkAndCanonicalizeTotal(modifier.total->amount);
            if (exception.hasException())
                return exception.releaseException();
        }

        for (auto& item : modifier.additionalDisplayItems) {
            auto exception = checkAndCanonicalizeAmount(item.amount);
            if (exception.hasException())
                return exception.releaseException();
        }

        String serializedData;
        if (modifier.data) {
            auto scope = DECLARE_THROW_SCOPE(document.execState()->vm());
            serializedData = JSONStringify(document.execState(), modifier.data.get(), 0);
            if (scope.exception())
                return Exception { ExistingExceptionError };
        }
        serializedModifierData.uncheckedAppend(WTFMove(serializedData));
    }

    return adoptRef(*new PaymentRequest(document, WTFMove(options), WTFMove(details), WTFMove(serializedModifierData), WTFMove(serializedMethodData), WTFMove(selectedShippingOption)));
}

PaymentRequest::PaymentRequest(Document& document, PaymentOptions&& options, PaymentDetailsInit&& details, Vector<String>&& serializedModifierData, Vector<Method>&& serializedMethodData, String&& selectedShippingOption)
    : ActiveDOMObject { &document }
    , m_options { WTFMove(options) }
    , m_details { WTFMove(details) }
    , m_serializedModifierData { WTFMove(serializedModifierData) }
    , m_serializedMethodData { WTFMove(serializedMethodData) }
    , m_shippingOption { WTFMove(selectedShippingOption) }
{
    suspendIfNeeded();
}

PaymentRequest::~PaymentRequest()
{
}

// https://www.w3.org/TR/payment-request/#show()-method
void PaymentRequest::show(ShowPromise&& promise)
{
    // FIXME: Reject promise with SecurityError if show() was not triggered by a user gesture.
    // Find a way to do this without breaking the payment-request web platform tests.

    if (m_state != State::Created) {
        promise.reject(Exception { InvalidStateError });
        return;
    }

    // FIXME: Reject promise with AbortError if PaymentCoordinator already has an active session.

    m_state = State::Interactive;
    ASSERT(!m_showPromise);
    m_showPromise = WTFMove(promise);

    // The spec requires these steps to be run after returning `promise` to the caller.
    RunLoop::main().dispatch([this, protectedThis = makeRef(*this)] {
        finishShowing();
    });
}

void PaymentRequest::finishShowing()
{
    ASSERT(m_showPromise);

    for (auto& paymentMethod : m_serializedMethodData) {
        auto scope = DECLARE_THROW_SCOPE(scriptExecutionContext()->vm());
        JSC::JSValue data = JSONParse(scriptExecutionContext()->execState(), paymentMethod.serializedData);
        if (scope.exception()) {
            m_showPromise->reject(Exception { ExistingExceptionError });
            return;
        }

        // FIXME: If there is a payment handler that can support this payment method, allow it to
        // convert the serialized data (propagating any exceptions that might be thrown) and add it
        // to a list of handlers.
        UNUSED(data);
    }

    // FIXME: If the list of handlers is non-empty, present the payment UI instead of rejecting.
    m_showPromise->reject(Exception { NotSupportedError });
}

// https://www.w3.org/TR/payment-request/#abort()-method
ExceptionOr<void> PaymentRequest::abort(AbortPromise&& promise)
{
    if (m_state != State::Interactive)
        return Exception { InvalidStateError };

    ASSERT(m_showPromise);
    ASSERT(!m_abortPromise);
    m_abortPromise = WTFMove(promise);

    // The spec requires these steps to be run after returning `promise` to the caller.
    RunLoop::main().dispatch([this, protectedThis = makeRef(*this)] {
        m_state = State::Closed;
        m_showPromise->reject(Exception { AbortError });
        m_abortPromise->resolve();
    });

    return { };
}

// https://www.w3.org/TR/payment-request/#canmakepayment()-method
void PaymentRequest::canMakePayment(CanMakePaymentPromise&& promise)
{
    if (m_state != State::Created) {
        promise.reject(Exception { InvalidStateError });
        return;
    }

    m_canMakePaymentPromise = WTFMove(promise);

    // The spec requires these steps to be run after returning `promise` to the caller.
    RunLoop::main().dispatch([this, protectedThis = makeRef(*this)] {
        // FIXME: Resolve the promise with true if we can support any of the payment methods in m_serializedMethodData.
        m_canMakePaymentPromise->resolve(false);
    });
}

const String& PaymentRequest::id() const
{
    return m_details.id;
}

std::optional<PaymentShippingType> PaymentRequest::shippingType() const
{
    if (m_options.requestShipping)
        return m_options.shippingType;
    return std::nullopt;
}

} // namespace WebCore

#endif // ENABLE(PAYMENT_REQUEST)
