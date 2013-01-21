/**************************************************************
 * 
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 * 
 *   http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 * 
 *************************************************************/

#include "precompiled_sfx2.hxx"

#include "PanelTitleBar.hxx"

#include "Paint.hxx"
#include "Panel.hxx"
#include "sfx2/sidebar/Theme.hxx"

#include <tools/svborder.hxx>
#include <vcl/gradient.hxx>

namespace sfx2 { namespace sidebar {


static const sal_Int32 gaLeftIconPadding (5);
static const sal_Int32 gaRightIconPadding (5);


PanelTitleBar::PanelTitleBar (
    const ::rtl::OUString& rsTitle,
    Window* pParentWindow,
    Panel* pPanel)
    : TitleBar(rsTitle, pParentWindow),
      mbIsLeftButtonDown(false),
      mpPanel(pPanel)
{
    OSL_ASSERT(mpPanel != NULL);
}




PanelTitleBar::~PanelTitleBar (void)
{
}




Rectangle PanelTitleBar::GetTitleArea (const Rectangle& rTitleBarBox)
{
    if (mpPanel != NULL)
    {
        Image aImage (mpPanel->IsExpanded()
            ? Theme::GetImage(Theme::Image_Expand)
            : Theme::GetImage(Theme::Image_Collapse));
        return Rectangle(
            aImage.GetSizePixel().Width() + gaLeftIconPadding + gaRightIconPadding,
            rTitleBarBox.Top(),
            rTitleBarBox.Right(),
            rTitleBarBox.Bottom());
    }
    else
        return rTitleBarBox;
}




void PanelTitleBar::PaintDecoration (const Rectangle& rTitleBarBox)
{
    if (mpPanel != NULL)
    {
        Image aImage (mpPanel->IsExpanded()
            ? Theme::GetImage(Theme::Image_Expand)
            : Theme::GetImage(Theme::Image_Collapse));
        const Point aTopLeft (
            gaLeftIconPadding,
            (GetSizePixel().Height()-aImage.GetSizePixel().Height())/2);
        DrawImage(aTopLeft, aImage);
    }
}




Paint PanelTitleBar::GetBackgroundPaint (void)
{
    return Theme::GetPaint(Theme::Paint_PanelTitleBarBackground);
}




Color PanelTitleBar::GetTextColor (void)
{
    return Theme::GetColor(Theme::Color_PanelTitleFont);
}




void PanelTitleBar::MouseButtonDown (const MouseEvent& rMouseEvent)
{
    if (rMouseEvent.IsLeft())
    {
        mbIsLeftButtonDown = true;
        CaptureMouse();
    }
}




void PanelTitleBar::MouseButtonUp (const MouseEvent& rMouseEvent)
{
    if (IsMouseCaptured())
        ReleaseMouse();
    
    if (rMouseEvent.IsLeft())
    {
        if (mbIsLeftButtonDown)
        {
            if (mpPanel != NULL)
            {
                mpPanel->SetExpanded( ! mpPanel->IsExpanded());
                Invalidate();
            }
        }
    }
    if (mbIsLeftButtonDown)
        mbIsLeftButtonDown = false;
}



} } // end of namespace sfx2::sidebar
