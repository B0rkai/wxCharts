/*
    Copyright (c) 2016-2021 Xavier Leclercq

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
    IN THE SOFTWARE.
*/

/*
    Part of this file were copied from the Chart.js project (http://chartjs.org/)
    and translated into C++.

    The files of the Chart.js project have the following copyright and license.

    Copyright (c) 2013-2016 Nick Downie
    Released under the MIT license
    https://github.com/nnnick/Chart.js/blob/master/LICENSE.md
*/

#include "wxcharttooltip.h"
#include "wxchartbackground.h"
#include "wxchartsutilities.h"
#include <wx/brush.h>
#include <wx/vector.h>

wxChartTooltip::wxChartTooltip(const wxPoint2DDouble &position,
                               const wxString &text)
    : m_position(position), m_provider(new wxChartTooltipProviderStatic("", text, *wxBLACK))
{
}

wxChartTooltip::wxChartTooltip(const wxPoint2DDouble &position,
                               const wxSharedPtr<wxChartTooltipProvider> provider)
    : m_position(position), m_provider(provider)
{
}

void wxChartTooltip::Draw(wxGraphicsContext &gc)
{
    wxString text = m_provider->GetTooltipText();

    wxFont font(wxSize(0, m_options.GetFontSize()),
        m_options.GetFontFamily(), m_options.GetFontStyle(), wxFONTWEIGHT_NORMAL);

    // BankAccount-added: measure and draw each line separately - the text can contain embedded
    // '\n' (e.g. a pie slice's category/total/average on separate lines - see
    // wxChartSliceData::SetTooltipTextOverride()), and wxGraphicsContext's GetTextExtent()/
    // DrawText() aren't a given to lay out multi-line text correctly across every backend on
    // their own, so this lays it out itself: width is the widest line, height is the line count
    // times one line's height, and each line is drawn at its own vertical offset.
    wxVector<wxString> lines;
    {
        wxString remaining = text;
        for (;;)
        {
            int pos = remaining.Find(wxT('\n'));
            if (pos == wxNOT_FOUND)
            {
                lines.push_back(remaining);
                break;
            }
            lines.push_back(remaining.Left(pos));
            remaining = remaining.Mid(pos + 1);
        }
    }

    wxDouble tooltipWidth = 0;
    wxDouble lineHeight = 0;
    for (const wxString& line : lines)
    {
        wxDouble w, h;
        wxChartsUtilities::GetTextSize(gc, font, line, w, h);
        if (w > tooltipWidth)
        {
            tooltipWidth = w;
        }
        if (h > lineHeight)
        {
            lineHeight = h;
        }
    }
    wxDouble tooltipHeight = lineHeight * lines.size();

    tooltipWidth += 2 * m_options.GetHorizontalPadding();
    tooltipHeight += 2 * m_options.GetVerticalPadding();


    wxDouble tooltipX = m_position.m_x - (tooltipWidth / 2);
    wxDouble tooltipY = m_position.m_y - tooltipHeight;


    wxChartBackground background(m_options.GetBackgroundOptions());
    background.Draw(tooltipX, tooltipY, tooltipWidth, tooltipHeight, gc);

    gc.SetFont(font, m_options.GetFontColor());
    for (size_t i = 0; i < lines.size(); ++i)
    {
        gc.DrawText(lines[i], tooltipX + m_options.GetHorizontalPadding(),
            tooltipY + m_options.GetVerticalPadding() + (lineHeight * i));
    }
}

const wxPoint2DDouble& wxChartTooltip::GetPosition() const
{
    return m_position;
}

const wxSharedPtr<wxChartTooltipProvider>& wxChartTooltip::GetProvider() const
{
    return m_provider;
}
