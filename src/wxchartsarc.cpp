/*
    Copyright (c) 2016-2019 Xavier Leclercq

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

#include "wxchartsarc.h"
#include <wx/pen.h>
#include <wx/brush.h>

wxChartsArc::wxChartsArc(wxDouble x,
                         wxDouble y, 
                         wxDouble startAngle,
                         wxDouble endAngle,
                         wxDouble outerRadius,
                         wxDouble innerRadius,
                         const wxString &tooltip,
                         const wxChartsArcOptions &options)
    : wxChartsElement(tooltip), m_x(x), m_y(y),
    m_startAngle(startAngle), m_endAngle(endAngle),
    m_outerRadius(outerRadius), m_innerRadius(innerRadius),
    m_options(options)
{
    // BankAccount-added: a slice spanning the whole circle (e.g. the only slice, at 100% share)
    // must stay distinguishable from a zero-width one. Independently wrapping m_startAngle and
    // m_endAngle into [0, 2*M_PI) below - needed for every ordinary slice - collapses an exact
    // 2*M_PI sweep into m_startAngle == m_endAngle (both wrap to the same value), indistinguishable
    // from an empty slice: Draw()'s path (built from start to end) and HitTest()'s betweenAngles
    // check then treat it as nothing rather than the whole circle. Detected from the sweep before
    // either angle is touched, and given the canonical, unambiguous [0, 2*M_PI] full-circle form -
    // which the normalization below leaves untouched (0 is not > 2*M_PI or < 0; 2*M_PI is not
    // > 2*M_PI or < 0), so no separate branch is needed for it.
    if ((m_endAngle - m_startAngle) >= (2 * M_PI - 1e-9))
    {
        m_startAngle = 0;
        m_endAngle = 2 * M_PI;
    }
    if (m_startAngle > (2 * M_PI))
    {
        m_startAngle -= 2 * M_PI;
    }
    // BankAccount-added: a chart whose first slice starts at a non-zero angle (e.g.
    // wxDoughnutAndPieChartBase::DoFit()'s -M_PI/2 for a 12 o'clock start - see CLAUDE.md's
    // wxCharts note) produces a negative raw angle for however many leading slices haven't yet
    // reached angle 0 - previously left un-normalized here, which broke HitTest()'s angle
    // comparison (it normalizes the mouse angle into [0, 2*M_PI) but compared it against this
    // still-negative m_startAngle/m_endAngle), so hovering those slices in the region before angle
    // 0 (e.g. between 12 and 3 o'clock for the very first slice) found no tooltip.
    if (m_startAngle < 0)
    {
        m_startAngle += 2 * M_PI;
    }
    if (m_endAngle > (2 * M_PI))
    {
        m_endAngle -= 2 * M_PI;
    }
    if (m_endAngle < 0)
    {
        m_endAngle += 2 * M_PI;
    }
}

void wxChartsArc::Draw(wxGraphicsContext &gc) const
{
    wxGraphicsPath path = gc.CreatePath();

    if (m_innerRadius > 0)
    {
        path.AddArc(m_x, m_y, m_innerRadius, m_startAngle, m_endAngle, true);
        path.AddArc(m_x, m_y, m_outerRadius, m_endAngle, m_startAngle, false);
    }
    else
    {
        path.AddArc(m_x, m_y, m_outerRadius, m_endAngle, m_startAngle, false);
        path.AddLineToPoint(m_x, m_y);
    }

    path.CloseSubpath();

    wxBrush brush(m_options.GetFillColor());
    gc.SetBrush(brush);
    gc.FillPath(path);

    wxPen pen(*wxWHITE, m_options.GetOutlineWidth());
    gc.SetPen(pen);
    gc.StrokePath(path);
}

bool wxChartsArc::HitTest(const wxPoint &point) const
{
    wxDouble distanceFromXCenter = point.x - m_x;
    wxDouble distanceFromYCenter = point.y - m_y;
    wxDouble radialDistanceFromCenter = sqrt((distanceFromXCenter * distanceFromXCenter) + (distanceFromYCenter * distanceFromYCenter));

    wxDouble angle = atan2(distanceFromYCenter, distanceFromXCenter);
    if (angle < 0)
    {
        angle += 2 * M_PI;
    }

    // Calculate wether the angle is between the start and the end angle
    bool betweenAngles = false;
    if (m_startAngle <= m_endAngle)
    {
        betweenAngles = ((angle >= m_startAngle) && (angle <= m_endAngle));
    }
    else
    {
        betweenAngles =
            (
                ((angle >= m_startAngle) && (angle <= (2 * M_PI)))
                ||
                ((angle >= 0) && (angle <= m_endAngle))
            );
    }

    // Ensure within the outside of the arc centre, but inside arc outer
    bool withinRadius = ((radialDistanceFromCenter >= m_innerRadius) && (radialDistanceFromCenter <= m_outerRadius));

    return (betweenAngles && withinRadius);
}

wxPoint2DDouble wxChartsArc::GetTooltipPosition() const
{
    // BankAccount-added: m_endAngle < m_startAngle means this slice wraps past angle 0 (see the
    // constructor's comment on why a non-zero chart start angle can produce that) - without this,
    // the plain midpoint below landed on the wrong side of the circle for a wrapping slice, since
    // (m_endAngle - m_startAngle) went negative instead of representing the slice's actual span.
    wxDouble endAngle = m_endAngle;
    if (endAngle < m_startAngle)
    {
        endAngle += 2 * M_PI;
    }
    wxDouble centreAngle = m_startAngle + (endAngle - m_startAngle) / 2;
    wxDouble rangeFromCentre = m_innerRadius + (m_outerRadius - m_innerRadius) / 2;
    wxDouble x = m_x + cos(centreAngle) * rangeFromCentre;
    wxDouble y = m_y + sin(centreAngle) * rangeFromCentre;
    return wxPoint2DDouble(x, y);
}

void wxChartsArc::SetCenter(wxDouble x, wxDouble y)
{
    m_x = x;
    m_y = y;
}

void wxChartsArc::SetAngles(wxDouble startAngle, wxDouble endAngle)
{
    m_startAngle = startAngle;
    m_endAngle = endAngle;
    // BankAccount-added: same full-circle special case as the constructor above (see its comment)
    // - DoFit() calls SetAngles() directly for every slice after the first, so a chart whose only
    // non-empty slice isn't the first one (e.g. earlier slices folded away as zero-value) would
    // still hit this through SetAngles() rather than the constructor.
    if ((m_endAngle - m_startAngle) >= (2 * M_PI - 1e-9))
    {
        m_startAngle = 0;
        m_endAngle = 2 * M_PI;
    }
    if (m_startAngle > (2 * M_PI))
    {
        m_startAngle -= 2 * M_PI;
    }
    // BankAccount-added: see the constructor's comment - same negative-angle normalization needed
    // here, since wxDoughnutAndPieChartBase::DoFit() calls SetAngles() directly rather than going
    // through the constructor for every slice after the first.
    if (m_startAngle < 0)
    {
        m_startAngle += 2 * M_PI;
    }
    if (m_endAngle > (2 * M_PI))
    {
        m_endAngle -= 2 * M_PI;
    }
    if (m_endAngle < 0)
    {
        m_endAngle += 2 * M_PI;
    }
}

void wxChartsArc::SetRadiuses(wxDouble outerRadius, wxDouble innerRadius)
{
    m_outerRadius = outerRadius;
    m_innerRadius = innerRadius;
}

const wxChartsArcOptions& wxChartsArc::GetOptions() const
{
    return m_options;
}
