/*
 * Copyright (C) 2004, 2005 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006, 2007 Rob Buis <buis@kde.org>
 * Copyright (C) Research In Motion Limited 2011. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#pragma once

#include "CSSValue.h"
#include "Color.h"

namespace WebCore {

class RGBColor;

class SVGColor : public CSSValue {
public:
    enum SVGColorType {
        SVG_COLORTYPE_UNKNOWN = 0,
        SVG_COLORTYPE_RGBCOLOR = 1,
        SVG_COLORTYPE_RGBCOLOR_ICCCOLOR = 2,
        SVG_COLORTYPE_CURRENTCOLOR = 3
    };

    static Ref<SVGColor> createFromString(const String& rgbColor)
    {
        auto color = adoptRef(*new SVGColor(SVG_COLORTYPE_RGBCOLOR));
        color.get().setColor(colorFromRGBColorString(rgbColor));
        return color;
    }

    static Ref<SVGColor> createFromColor(const Color& rgbColor)
    {
        auto color = adoptRef(*new SVGColor(SVG_COLORTYPE_RGBCOLOR));
        color.get().setColor(rgbColor);
        return color;
    }

    static Ref<SVGColor> createCurrentColor()
    {
        return adoptRef(*new SVGColor(SVG_COLORTYPE_CURRENTCOLOR));
    }

    const Color& color() const { return m_color; }
    const SVGColorType& colorType() const { return m_colorType; }
    Ref<RGBColor> rgbColor() const;

    static Color colorFromRGBColorString(const String&);

    ExceptionOr<void> setRGBColor(const String&);
    ExceptionOr<void> setRGBColorICCColor(const String& rgbColor, const String& iccColor);
    ExceptionOr<void> setColor(unsigned short colorType, const String& rgbColor, const String& iccColor);

    String customCSSText() const;

    Ref<SVGColor> cloneForCSSOM() const;

    bool equals(const SVGColor&) const;

protected:
    friend class CSSComputedStyleDeclaration;

    SVGColor(ClassType, SVGColorType);
    SVGColor(ClassType, const SVGColor& cloneFrom);

    void setColor(const Color& color) { m_color = color; }
    void setColorType(const SVGColorType& type) { m_colorType = type; }

private:
    explicit SVGColor(SVGColorType);

    Color m_color;
    SVGColorType m_colorType;
};

} // namespace WebCore

SPECIALIZE_TYPE_TRAITS_CSS_VALUE(SVGColor, isSVGColor())
