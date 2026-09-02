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

#include "wxdoughnutandpiechartbase.h"

wxPieChartData::wxPieChartData()
{
}

wxPieChartData::ptr wxPieChartData::make_shared()
{
    return ptr(new wxPieChartData());
}

const wxVector<wxChartSliceData>& wxPieChartData::GetSlices() const
{
    return m_value;
}

void wxPieChartData::AppendSlice(const wxChartSliceData &slice)
{
    Add(slice);
    Notify();
}

void wxPieChartData::UpdateSlices(const wxVector<wxChartSliceData> &slices)
{
    m_value.clear();
    AddSlices(slices);
}

void wxPieChartData::AddSlices(const wxVector<wxChartSliceData> &slices)
{
    for (const auto &slice : slices)
        Add(slice);

    Notify();
}

void wxPieChartData::Add(const wxChartSliceData &slice)
{
    // BankAccount-added: linear search instead of a map lookup - m_value is now an
    // (append-order-preserving) wxVector rather than a std::map<wxString, wxChartSliceData>,
    // see the header. Slice counts here are at most a few dozen/hundred categories, so this
    // stays cheap.
    for (size_t i = 0; i < m_value.size(); ++i)
    {
        if (m_value[i].GetLabel() == slice.GetLabel())
        {
            m_value[i].SetValue(m_value[i].GetValue() + slice.GetValue());
            return;
        }
    }
    m_value.push_back(slice);
}

wxDoughnutAndPieChartBase::SliceArc::SliceArc(const wxChartSliceData &slice,
                                              wxDouble x,
                                              wxDouble y,
                                              wxDouble startAngle,
                                              wxDouble endAngle,
                                              wxDouble outerRadius,
                                              wxDouble innerRadius,
                                              unsigned int strokeWidth)
    : wxChartsArc(x, y, startAngle, endAngle, outerRadius, innerRadius, 
        slice.GetTooltipText(), wxChartsArcOptions(strokeWidth, slice.GetColor())),
    m_value(slice.GetValue())
{
}

void wxDoughnutAndPieChartBase::SliceArc::Resize(const wxSize &size,
                                                 const wxDoughnutAndPieChartOptionsBase& options)
{
    wxDouble x = (size.GetX() / 2) - 2;
    wxDouble y = (size.GetY() / 2) - 2;
    wxDouble outerRadius = ((x < y) ? x : y) - (GetOptions().GetOutlineWidth() / 2);
    wxDouble innerRadius = outerRadius * ((wxDouble)options.GetPercentageInnerCutout()) / 100;

    SetCenter(x, y);
    SetRadiuses(outerRadius, innerRadius);
}

wxDouble wxDoughnutAndPieChartBase::SliceArc::GetValue() const
{
    return m_value;
}

wxDoughnutAndPieChartBase::wxDoughnutAndPieChartBase(wxPieChartData::ptr data)
    : m_data(data), m_total(0)
{
}

wxDoughnutAndPieChartBase::wxDoughnutAndPieChartBase(const wxPieChartData::ptr data,
                                                     const wxSize &size)
    : m_data(data), m_size(size), m_total(0)
{
}

void wxDoughnutAndPieChartBase::SetData(const wxVector<wxChartSliceData> &data)
{
    m_slices.resize(data.size());
    m_total = 0;
	size_t i = 0;
    for (const auto &slice : data)
    {
        m_total += slice.GetValue();
        wxDouble x = (m_size.GetX() / 2) - 2;
        wxDouble y = (m_size.GetY() / 2) - 2;
        wxDouble outerRadius = ((x < y) ? x : y) - (GetOptions().GetSliceStrokeWidth() / 2);
        wxDouble innerRadius = outerRadius * ((wxDouble)GetOptions().GetPercentageInnerCutout()) / 100;

        SliceArc::ptr newSlice = SliceArc::ptr(new SliceArc(slice,
                                               x, y, 0, 0, outerRadius, innerRadius, GetOptions().GetSliceStrokeWidth()));
        m_slices[i++] = newSlice;
    }
}

void wxDoughnutAndPieChartBase::DoSetSize(const wxSize &size)
{
    m_size = size;
}

void wxDoughnutAndPieChartBase::DoFit()
{
    SetData(m_data->GetSlices());

    for (size_t i = 0; i < m_slices.size(); ++i)
    {
        m_slices[i]->Resize(m_size, GetOptions());
    }

    // BankAccount-added: was 0.0 (angle 0 = the positive x-axis, i.e. 3 o'clock, since angles here
    // are screen-space with y increasing downward). -M_PI / 2 starts the first slice at 12 o'clock
    // instead, matching the conventional pie-chart layout (and wxPolarAreaChartOptions, which
    // already exposes SetStartAngle() for the same reason - only this base class had no equivalent
    // knob, since it always drew from the same hardcoded 0.0).
    wxDouble startAngle = -M_PI / 2;
    for (size_t i = 0; i < m_slices.size(); ++i)
    {
        SliceArc& currentSlice = *m_slices[i];

        wxDouble endAngle = startAngle + CalculateCircumference(currentSlice.GetValue());
        currentSlice.SetAngles(startAngle, endAngle);
        startAngle = endAngle;
    }
}

void wxDoughnutAndPieChartBase::DoDraw(wxGraphicsContext &gc,
                                       bool suppressTooltips)
{
    Fit();

    for (size_t i = 0; i < m_slices.size(); ++i)
    {
        m_slices[i]->Draw(gc);
    }

    if (!suppressTooltips)
    {
        DrawTooltips(gc);
    }
}

wxSharedPtr<wxVector<const wxChartsElement*>> wxDoughnutAndPieChartBase::GetActiveElements(const wxPoint &point)
{
    wxSharedPtr<wxVector<const wxChartsElement*>> activeElements(new wxVector<const wxChartsElement*>());
    for (size_t i = 0; i < m_slices.size(); ++i)
    {
        if (m_slices[i]->HitTest(point))
        {
            activeElements->push_back(m_slices[i].get());
        }
    }
    return activeElements;
}

wxDouble wxDoughnutAndPieChartBase::CalculateCircumference(wxDouble value)
{
    if (m_total > 0)
    {
        return (value * 2 * M_PI / m_total);
    }
    else
    {
        return 0;
    }
}
