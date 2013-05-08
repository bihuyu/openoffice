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


#include "precompiled_reportdesign.hxx"

#include "ViewsWindow.hxx"
#include "ScrollHelper.hxx"
#include "UndoActions.hxx"
#include "ReportWindow.hxx"
#include "DesignView.hxx"
#include <svtools/colorcfg.hxx>
#include "ReportController.hxx"
#include "UITools.hxx"
#include "RptDef.hxx"
#include "RptResId.hrc"
#include "SectionView.hxx"
#include "ReportSection.hxx"
#include "uistrings.hrc"
#include "rptui_slotid.hrc"
#include "dlgedclip.hxx"
#include "ColorChanger.hxx"
#include "RptObject.hxx"
#include "RptObject.hxx"
#include "ModuleHelper.hxx"
#include "EndMarker.hxx"
#include <svx/svdpagv.hxx>
#include <svx/unoshape.hxx>
#include <vcl/svapp.hxx>
#include <boost/bind.hpp>
#include <svx/svdlegacy.hxx>

#include "helpids.hrc"
#include <svx/svdundo.hxx>
#include <toolkit/helper/convert.hxx>
#include <algorithm>
#include <numeric>

namespace rptui
{
#define DEFAUL_MOVE_SIZE    100

using namespace ::com::sun::star;
using namespace ::comphelper;
// -----------------------------------------------------------------------------
bool lcl_getNewRectSize(const Rectangle& _aObjRect,long& _nXMov, long& _nYMov,SdrObject* _pObj,SdrView* _pView,sal_Int32 _nControlModification, bool _bBoundRects)
{
    bool bMoveAllowed = _nXMov != 0 || _nYMov != 0;
    if ( bMoveAllowed ) 
	{
        Rectangle aNewRect = _aObjRect;
        SdrObject* pOverlappedObj = NULL;
        do
        {
            aNewRect = _aObjRect;
            switch(_nControlModification)
            {
                case ControlModification::HEIGHT_GREATEST:
                case ControlModification::WIDTH_GREATEST:
                    aNewRect.setWidth(_nXMov);
                    aNewRect.setHeight(_nYMov);
                    break;
                default:
                    aNewRect.Move(_nXMov,_nYMov);
                    break;
            }
            if (dynamic_cast<OUnoObject*>(_pObj) != NULL || dynamic_cast<OOle2Obj*>(_pObj) != NULL)
            {                
                pOverlappedObj = isOver(
					basegfx::B2DRange(aNewRect.Left(), aNewRect.Top(), aNewRect.Right(), aNewRect.Bottom()),
					*_pObj->getSdrPageFromSdrObject(),
					*_pView,
					true,
					_pObj);

                if ( pOverlappedObj && _pObj != pOverlappedObj )
                {
                    const Rectangle aOverlappingRect(_bBoundRects 
						? sdr::legacy::GetBoundRect(*pOverlappedObj) 
						: sdr::legacy::GetSnapRect(*pOverlappedObj));
                    sal_Int32 nXTemp = _nXMov;
                    sal_Int32 nYTemp = _nYMov;
                    switch(_nControlModification)
                    {
                        case ControlModification::LEFT:
                            nXTemp += aOverlappingRect.Right() - aNewRect.Left(); 
                            bMoveAllowed = _nXMov != nXTemp;
                            break;
                        case ControlModification::RIGHT:
                            nXTemp += aOverlappingRect.Left() - aNewRect.Right();
                            bMoveAllowed = _nXMov != nXTemp;
                            break;
                        case ControlModification::TOP:
                            nYTemp += aOverlappingRect.Bottom() - aNewRect.Top();
                            bMoveAllowed = _nYMov != nYTemp;
                            break;
                        case ControlModification::BOTTOM:
                            nYTemp += aOverlappingRect.Top() - aNewRect.Bottom();
                            bMoveAllowed = _nYMov != nYTemp;
                            break;
                        case ControlModification::CENTER_HORIZONTAL:
                            if ( _aObjRect.Left() < aOverlappingRect.Left() )
                                nXTemp += aOverlappingRect.Left() - aNewRect.Left() - aNewRect.getWidth();
                            else
                                nXTemp += aOverlappingRect.Right() - aNewRect.Left();
                            bMoveAllowed = _nXMov != nXTemp;
                            break;
                        case ControlModification::CENTER_VERTICAL:
                            if ( _aObjRect.Top() < aOverlappingRect.Top() )
                                nYTemp += aOverlappingRect.Top() - aNewRect.Top() - aNewRect.getHeight();
                            else
                                nYTemp += aOverlappingRect.Bottom() - aNewRect.Top();
                            bMoveAllowed = _nYMov != nYTemp;
                            break;
                        case ControlModification::HEIGHT_GREATEST:
                        case ControlModification::WIDTH_GREATEST:
                            {
                                Rectangle aIntersectionRect = aNewRect.GetIntersection(aOverlappingRect);
                                if ( !aIntersectionRect.IsEmpty() )
                                {
                                    if ( _nControlModification == ControlModification::WIDTH_GREATEST )
                                    {
                                        if ( aNewRect.Left() < aIntersectionRect.Left() )
                                        {
                                            aNewRect.Right() = aIntersectionRect.Left();
                                        }
                                        else if ( aNewRect.Left() < aIntersectionRect.Right() )
                                        {
                                            aNewRect.Left() = aIntersectionRect.Right();
                                        }
                                    }
                                    else if ( _nControlModification == ControlModification::HEIGHT_GREATEST )
                                    {
                                        if ( aNewRect.Top() < aIntersectionRect.Top() )
                                        {
                                            aNewRect.Bottom() = aIntersectionRect.Top();
                                        }
                                        else if ( aNewRect.Top() < aIntersectionRect.Bottom() )
                                        {
                                            aNewRect.Top() = aIntersectionRect.Bottom();
                                        }
                                    }
                                    nYTemp = aNewRect.getHeight();
                                    bMoveAllowed = _nYMov != nYTemp;
                                    nXTemp = aNewRect.getWidth();
                                    bMoveAllowed = bMoveAllowed && _nXMov != nXTemp;
                                }
                            }
                            break;
                        default:
                            break;
                    }
                    
                    _nXMov = nXTemp;
                    _nYMov = nYTemp;
                }
                else
                    pOverlappedObj = NULL;
            }
        }
        while ( pOverlappedObj && bMoveAllowed );
	}
    return bMoveAllowed;
}
// -----------------------------------------------------------------------------

DBG_NAME( rpt_OViewsWindow );
OViewsWindow::OViewsWindow( OReportWindow* _pReportWindow) 
: Window( _pReportWindow,WB_DIALOGCONTROL)
,m_pParent(_pReportWindow)
,m_bInUnmark(sal_False)
{
	DBG_CTOR( rpt_OViewsWindow,NULL);
    SetPaintTransparent(sal_True);
    SetUniqueId(UID_RPT_VIEWSWINDOW);
	SetMapMode( MapMode( MAP_100TH_MM ) );
	m_aColorConfig.AddListener(this);
	ImplInitSettings();
}
// -----------------------------------------------------------------------------
OViewsWindow::~OViewsWindow()
{
	m_aColorConfig.RemoveListener(this);
	m_aSections.clear();

	DBG_DTOR( rpt_OViewsWindow,NULL);
}
// -----------------------------------------------------------------------------
void OViewsWindow::initialize()
{
	
}
// -----------------------------------------------------------------------------
void OViewsWindow::impl_resizeSectionWindow(OSectionWindow& _rSectionWindow,Point& _rStartPoint,bool _bSet)
{
	const uno::Reference< report::XSection> xSection = _rSectionWindow.getReportSection().getSection();

	Size aSectionSize = _rSectionWindow.LogicToPixel( Size( 0,xSection->getHeight() ) );
    aSectionSize.Width() = getView()->GetTotalWidth();
    
	const sal_Int32 nMinHeight = _rSectionWindow.getStartMarker().getMinHeight();
	if ( _rSectionWindow.getStartMarker().isCollapsed() || nMinHeight > aSectionSize.Height() )
    {
        aSectionSize.Height() = nMinHeight;
    }
    const StyleSettings& rSettings = GetSettings().GetStyleSettings();
    aSectionSize.Height() += (long)(rSettings.GetSplitSize() * (double)_rSectionWindow.GetMapMode().GetScaleY());

    if ( _bSet )
        _rSectionWindow.SetPosSizePixel(_rStartPoint,aSectionSize);

    _rStartPoint.Y() += aSectionSize.Height();
}

// -----------------------------------------------------------------------------
void OViewsWindow::resize(const OSectionWindow& _rSectionWindow)
{
    bool bSet = false;
    Point aStartPoint;
    TSectionsMap::iterator aIter = m_aSections.begin();
	TSectionsMap::iterator aEnd = m_aSections.end();
	for (;aIter != aEnd ; ++aIter)
	{
		const ::boost::shared_ptr<OSectionWindow> pSectionWindow = (*aIter);
        if ( pSectionWindow.get() == &_rSectionWindow )
        {
            aStartPoint = pSectionWindow->GetPosPixel();
            bSet = true;
        } // if ( pSectionWindow.get() == &_rSectionWindow )
        
        if ( bSet )
        {
            impl_resizeSectionWindow(*pSectionWindow.get(),aStartPoint,bSet);
            static sal_Int32 nIn = INVALIDATE_UPDATE | INVALIDATE_TRANSPARENT;
            pSectionWindow->getStartMarker().Invalidate( nIn ); // INVALIDATE_NOERASE |INVALIDATE_NOCHILDREN| INVALIDATE_TRANSPARENT 
            pSectionWindow->getEndMarker().Invalidate( nIn );
        }
    } // for (;aIter != aEnd ; ++aIter,++nPos)
    Fraction aStartWidth(long(REPORT_STARTMARKER_WIDTH));
    aStartWidth *= GetMapMode().GetScaleX();
    Size aOut = GetOutputSizePixel();
    aOut.Width() = aStartWidth;
    aOut = PixelToLogic(aOut);
    m_pParent->notifySizeChanged();
    
    Rectangle aRect(PixelToLogic(Point(0,0)),aOut);
}
//------------------------------------------------------------------------------
void OViewsWindow::Resize()
{
	Window::Resize();	
	if ( !m_aSections.empty() )
    {
        const Point aOffset(m_pParent->getThumbPos());
        Point aStartPoint(0,-aOffset.Y());	
	    TSectionsMap::iterator aIter = m_aSections.begin();
	    TSectionsMap::iterator aEnd = m_aSections.end();
	    for (sal_uInt16 nPos=0;aIter != aEnd ; ++aIter,++nPos)
	    {
		    const ::boost::shared_ptr<OSectionWindow> pSectionWindow = (*aIter);
            impl_resizeSectionWindow(*pSectionWindow.get(),aStartPoint,true);
	    } // for (;aIter != aEnd ; ++aIter)
    }
}
// -----------------------------------------------------------------------------
void OViewsWindow::Paint( const Rectangle& rRect )
{
    Window::Paint( rRect );
    
    Size aOut = GetOutputSizePixel();
    Fraction aStartWidth(long(REPORT_STARTMARKER_WIDTH));
    aStartWidth *= GetMapMode().GetScaleX();

    aOut.Width() -= (long)aStartWidth;
    aOut = PixelToLogic(aOut);
    
    Rectangle aRect(PixelToLogic(Point(aStartWidth,0)),aOut);
    Wallpaper aWall( m_aColorConfig.GetColorValue(::svtools::APPBACKGROUND).nColor );
    DrawWallpaper(aRect,aWall);
}
//------------------------------------------------------------------------------
void OViewsWindow::ImplInitSettings()
{	
    EnableChildTransparentMode( sal_True );
    SetBackground( );
	SetFillColor( Application::GetSettings().GetStyleSettings().GetDialogColor() );
	SetTextFillColor( Application::GetSettings().GetStyleSettings().GetDialogColor() );
}
//-----------------------------------------------------------------------------
void OViewsWindow::DataChanged( const DataChangedEvent& rDCEvt )
{
	Window::DataChanged( rDCEvt );

	if ( (rDCEvt.GetType() == DATACHANGED_SETTINGS) &&
		 (rDCEvt.GetFlags() & SETTINGS_STYLE) )
	{
		ImplInitSettings();
		Invalidate();
	}
}
//----------------------------------------------------------------------------
void OViewsWindow::addSection(const uno::Reference< report::XSection >& _xSection,const ::rtl::OUString& _sColorEntry,sal_uInt16 _nPosition)
{
    ::boost::shared_ptr<OSectionWindow> pSectionWindow( new OSectionWindow(this,_xSection,_sColorEntry) );
	m_aSections.insert(getIteratorAtPos(_nPosition) , TSectionsMap::value_type(pSectionWindow));
    m_pParent->setMarked(&pSectionWindow->getReportSection().getSectionView(),m_aSections.size() == 1);
	Resize();
}
//----------------------------------------------------------------------------
void OViewsWindow::removeSection(sal_uInt16 _nPosition)
{
	if ( _nPosition < m_aSections.size() )
	{
		TSectionsMap::iterator aPos = getIteratorAtPos(_nPosition);
		TSectionsMap::iterator aNew = getIteratorAtPos(_nPosition == 0 ? _nPosition+1: _nPosition - 1);

		m_pParent->getReportView()->UpdatePropertyBrowserDelayed((*aNew)->getReportSection().getSectionView());
		
		m_aSections.erase(aPos);
		Resize();
	} // if ( _nPosition < m_aSections.size() )
}
//------------------------------------------------------------------------------
void OViewsWindow::toggleGrid(sal_Bool _bVisible)
{
	::std::for_each(m_aSections.begin(),m_aSections.end(),
		::std::compose1(::boost::bind(&OReportSection::SetGridVisible,_1,_bVisible),TReportPairHelper()));
    ::std::for_each(m_aSections.begin(),m_aSections.end(),
		::std::compose1(::boost::bind(&OReportSection::Window::Invalidate,_1,INVALIDATE_NOERASE),TReportPairHelper()));
}
//------------------------------------------------------------------------------
sal_Int32 OViewsWindow::getTotalHeight() const
{
	sal_Int32 nHeight = 0;
	TSectionsMap::const_iterator aIter = m_aSections.begin();
	TSectionsMap::const_iterator aEnd = m_aSections.end();
	for (;aIter != aEnd ; ++aIter)
	{
		nHeight += (*aIter)->GetSizePixel().Height();
	}
    return nHeight;
}
//----------------------------------------------------------------------------
sal_uInt16 OViewsWindow::getSectionCount() const
{
	return static_cast<sal_uInt16>(m_aSections.size());
}
//----------------------------------------------------------------------------
void OViewsWindow::SetInsertObj( sal_uInt16 eObj,const ::rtl::OUString& _sShapeType )
{
	TSectionsMap::iterator aIter = m_aSections.begin();
	TSectionsMap::iterator aEnd = m_aSections.end();
	for (;aIter != aEnd ; ++aIter)
		(*aIter)->getReportSection().getSectionView().setSdrObjectCreationInfo(SdrObjectCreationInfo(eObj, ReportInventor));

    m_sShapeType = _sShapeType;
}
//----------------------------------------------------------------------------
rtl::OUString OViewsWindow::GetInsertObjString() const
{
	return m_sShapeType;
}

//------------------------------------------------------------------------------
void OViewsWindow::SetMode( DlgEdMode eNewMode )
{
	::std::for_each(m_aSections.begin(),m_aSections.end(),
		::std::compose1(::boost::bind(&OReportSection::SetMode,_1,eNewMode),TReportPairHelper()));
}
//----------------------------------------------------------------------------
sal_Bool OViewsWindow::HasSelection() const
{
	TSectionsMap::const_iterator aIter = m_aSections.begin();
	TSectionsMap::const_iterator aEnd = m_aSections.end();
	for (;aIter != aEnd && !(*aIter)->getReportSection().getSectionView().areSdrObjectsSelected(); ++aIter)
		;
	return aIter != aEnd; 
}
//----------------------------------------------------------------------------
void OViewsWindow::Delete()
{
	m_bInUnmark = sal_True;
	::std::for_each(m_aSections.begin(),m_aSections.end(),
		::std::compose1(::boost::mem_fn(&OReportSection::Delete),TReportPairHelper()));
	m_bInUnmark = sal_False;
}
//----------------------------------------------------------------------------
void OViewsWindow::Copy()
{
    uno::Sequence< beans::NamedValue > aAllreadyCopiedObjects;
    ::std::for_each(m_aSections.begin(),m_aSections.end(),
        ::std::compose1(::boost::bind(&OReportSection::Copy,_1,::boost::ref(aAllreadyCopiedObjects)),TReportPairHelper()));

    //TSectionsMap::iterator aIter = m_aSections.begin();
    //TSectionsMap::iterator aEnd = m_aSections.end();
    //for (; aIter != aEnd; ++aIter)
    //    (*aIter)->getReportSection().Copy(aAllreadyCopiedObjects);
    OReportExchange* pCopy = new OReportExchange(aAllreadyCopiedObjects);
    uno::Reference< datatransfer::XTransferable> aEnsureDelete = pCopy;
    pCopy->CopyToClipboard(this);
}
//----------------------------------------------------------------------------
void OViewsWindow::Paste()
{   
    TransferableDataHelper aTransferData(TransferableDataHelper::CreateFromSystemClipboard(this));
    OReportExchange::TSectionElements aCopies = OReportExchange::extractCopies(aTransferData);
    if ( aCopies.getLength() > 1 )
        ::std::for_each(m_aSections.begin(),m_aSections.end(),
		    ::std::compose1(::boost::bind(&OReportSection::Paste,_1,aCopies,false),TReportPairHelper()));
    else
    {
		::boost::shared_ptr<OSectionWindow> pMarkedSection = getMarkedSection();
		if ( pMarkedSection )
			pMarkedSection->getReportSection().Paste(aCopies,true);
    }
}
// ---------------------------------------------------------------------------
::boost::shared_ptr<OSectionWindow> OViewsWindow::getSectionWindow(const uno::Reference< report::XSection>& _xSection) const
{
	OSL_ENSURE(_xSection.is(),"Section is NULL!");

	::boost::shared_ptr<OSectionWindow> pSectionWindow;
	TSectionsMap::const_iterator aIter = m_aSections.begin();
	TSectionsMap::const_iterator aEnd = m_aSections.end();
	for (; aIter != aEnd ; ++aIter)
	{
        if ((*aIter)->getReportSection().getSection() == _xSection)
        {
            pSectionWindow = (*aIter);
            break;
        }
    }
    
    return pSectionWindow;
}

//----------------------------------------------------------------------------
::boost::shared_ptr<OSectionWindow> OViewsWindow::getMarkedSection(NearSectionAccess nsa) const
{
	::boost::shared_ptr<OSectionWindow> pRet;
	TSectionsMap::const_iterator aIter = m_aSections.begin();
	TSectionsMap::const_iterator aEnd = m_aSections.end();
	sal_uInt32 nCurrentPosition = 0;	
	for (; aIter != aEnd ; ++aIter)
	{
		if ( (*aIter)->getStartMarker().isMarked() )
        {
            if (nsa == CURRENT)
            {
                pRet = (*aIter);
                break;
            } 
            else if ( nsa == PREVIOUS )
            {
				if (nCurrentPosition > 0)
				{
					pRet = (*(--aIter));
	                if (pRet == NULL)
		            {
			            pRet = (*m_aSections.begin());
				    }
				}
				else
				{
					// if we are out of bounds return the first one
					pRet = (*m_aSections.begin());
				}
                break;
            }
            else if ( nsa == POST )
            {
				sal_uInt32 nSize = m_aSections.size();
				if ((nCurrentPosition + 1) < nSize)
				{
					pRet = *(++aIter);
	                if (pRet == NULL)
		            {
			            pRet = (*(--aEnd));
				    }
				}
				else
				{
					// if we are out of bounds return the last one
					pRet = (*(--aEnd));
				}
                break;
            }
        } // ( (*aIter).second->isMarked() )
		++nCurrentPosition;
	} // for (; aIter != aEnd ; ++aIter)
    
	return pRet;
}
// -------------------------------------------------------------------------
void OViewsWindow::markSection(const sal_uInt16 _nPos) 
{
    if ( _nPos < m_aSections.size() )
        m_pParent->setMarked(m_aSections[_nPos]->getReportSection().getSection(),sal_True);
}
//----------------------------------------------------------------------------
sal_Bool OViewsWindow::IsPasteAllowed() const
{
    TransferableDataHelper aTransferData( TransferableDataHelper::CreateFromSystemClipboard( const_cast< OViewsWindow* >( this ) ) );
    return aTransferData.HasFormat(OReportExchange::getDescriptorFormatId());
}
//-----------------------------------------------------------------------------
void OViewsWindow::SelectAll(const sal_uInt16 _nObjectType)
{
	m_bInUnmark = sal_True;
	::std::for_each(m_aSections.begin(),m_aSections.end(),
		::std::compose1(::boost::bind(::boost::mem_fn(&OReportSection::SelectAll),_1,_nObjectType),TReportPairHelper()));
	m_bInUnmark = sal_False;
}
//-----------------------------------------------------------------------------
void OViewsWindow::unmarkAllObjects(OSectionView* _pSectionView)
{
	if ( !m_bInUnmark )
	{
		m_bInUnmark = sal_True;
		TSectionsMap::iterator aIter = m_aSections.begin();
		TSectionsMap::iterator aEnd = m_aSections.end();
		for (; aIter != aEnd ; ++aIter)
		{
			if ( &(*aIter)->getReportSection().getSectionView() != _pSectionView )
            {
                (*aIter)->getReportSection().deactivateOle();
				(*aIter)->getReportSection().getSectionView().UnmarkAllObj();
            }
		} // for (; aIter != aEnd ; ++aIter)
		m_bInUnmark = sal_False;
	}
}
//-----------------------------------------------------------------------------
/*
::boost::shared_ptr<OSectionWindow>	OViewsWindow::getReportSection(const uno::Reference< report::XSection >& _xSection)
{
	OSL_ENSURE(_xSection.is(),"Section is NULL!");
	::boost::shared_ptr<OSectionWindow>	pRet;
	TSectionsMap::iterator aIter = m_aSections.begin();
	TSectionsMap::iterator aEnd = m_aSections.end();
	for (; aIter != aEnd ; ++aIter)
	{
		if ( (*aIter)->getReportSection().getSection() == _xSection )
		{
			pRet = (*aIter);
			break;
		} // if ( (*aIter)->getSection() == _xSection )
	} // for (; aIter != aEnd ; ++aIter)
	return pRet;
}
*/
// -----------------------------------------------------------------------
void OViewsWindow::ConfigurationChanged( utl::ConfigurationBroadcaster*, sal_uInt32)
{
	ImplInitSettings();
	Invalidate();
}
// -----------------------------------------------------------------------------
void OViewsWindow::MouseButtonDown( const MouseEvent& rMEvt )
{
	if ( rMEvt.IsLeft() )
	{
        GrabFocus();
		const uno::Sequence< beans::PropertyValue> aArgs;
		getView()->getReportView()->getController().executeChecked(SID_SELECT_REPORT,aArgs);
	}
    Window::MouseButtonDown(rMEvt);
}
//----------------------------------------------------------------------------
void OViewsWindow::showRuler(sal_Bool _bShow)
{
	::std::for_each(m_aSections.begin(),m_aSections.end(),
		::std::compose1(::boost::bind(&OStartMarker::showRuler,_1,_bShow),TStartMarkerHelper()));
    ::std::for_each(m_aSections.begin(),m_aSections.end(),
        ::std::compose1(::boost::bind(&OStartMarker::Window::Invalidate,_1,sal_uInt16(INVALIDATE_NOERASE)),TStartMarkerHelper()));
}
//----------------------------------------------------------------------------
void OViewsWindow::MouseButtonUp( const MouseEvent& rMEvt )
{
	if ( rMEvt.IsLeft() )
	{
		TSectionsMap::iterator aIter = m_aSections.begin();
		TSectionsMap::iterator aEnd = m_aSections.end();
		for (;aIter != aEnd ; ++aIter)
		{
			if ( (*aIter)->getReportSection().getSectionView().areSdrObjectsSelected() )
			{
				(*aIter)->getReportSection().MouseButtonUp(rMEvt);
				break;
			}
		}

        // remove special insert mode
        for (aIter = m_aSections.begin();aIter != aEnd ; ++aIter)
        {
            (*aIter)->getReportSection().getPage()->resetSpecialMode();
        }        
	}
}
//------------------------------------------------------------------------------
sal_Bool OViewsWindow::handleKeyEvent(const KeyEvent& _rEvent)
{
	sal_Bool bRet = sal_False;
	TSectionsMap::iterator aIter = m_aSections.begin();
	TSectionsMap::iterator aEnd = m_aSections.end();
	for (;aIter != aEnd ; ++aIter)
	{
		//if ( (*aIter).getReportSection().getSectionView().->areSdrObjectsSelected() )
        if ( (*aIter)->getStartMarker().isMarked() )
		{
			bRet = (*aIter)->getReportSection().handleKeyEvent(_rEvent);
		}
	}
	return bRet;
}
//----------------------------------------------------------------------------
OViewsWindow::TSectionsMap::iterator OViewsWindow::getIteratorAtPos(sal_uInt16 _nPos)
{
	TSectionsMap::iterator aRet = m_aSections.end();
	if ( _nPos < m_aSections.size() )
		aRet = m_aSections.begin() + _nPos;
	return aRet;
}
//------------------------------------------------------------------------
void OViewsWindow::setMarked(OSectionView* _pSectionView,sal_Bool _bMark)
{
    OSL_ENSURE(_pSectionView != NULL,"SectionView is NULL!");
    if ( _pSectionView )
        setMarked(_pSectionView->getReportSection()->getSection(),_bMark);
}
//------------------------------------------------------------------------
void OViewsWindow::setMarked(const uno::Reference< report::XSection>& _xSection,sal_Bool _bMark)
{
	TSectionsMap::iterator aIter = m_aSections.begin();
	TSectionsMap::iterator aEnd = m_aSections.end();
	for (; aIter != aEnd ; ++aIter)
	{
        if ( (*aIter)->getReportSection().getSection() != _xSection )
        {
            (*aIter)->setMarked(sal_False);
        }
		else if ( (*aIter)->getStartMarker().isMarked() != _bMark )
		{
			(*aIter)->setMarked(_bMark);
		}
	}
}
//------------------------------------------------------------------------
void OViewsWindow::setMarked(const uno::Sequence< uno::Reference< report::XReportComponent> >& _aShapes,sal_Bool _bMark)
{
    bool bFirst = true;
    const uno::Reference< report::XReportComponent>* pIter = _aShapes.getConstArray();
    const uno::Reference< report::XReportComponent>* pEnd  = pIter + _aShapes.getLength();
    for(;pIter != pEnd;++pIter)
    {
        const uno::Reference< report::XSection> xSection = (*pIter)->getSection();
        if ( xSection.is() )
        {
            if ( bFirst )
            {
                bFirst = false;
                m_pParent->setMarked(xSection,_bMark);
            }
            ::boost::shared_ptr<OSectionWindow>	pSectionWindow = getSectionWindow(xSection);
            if ( pSectionWindow )
            {
                SvxShape* pShape = SvxShape::getImplementation( *pIter );
                SdrObject* pObject = pShape ? pShape->GetSdrObject() : NULL;
                OSL_ENSURE( pObject, "OViewsWindow::setMarked: no SdrObject for the shape!" );
                if ( pObject )
                    pSectionWindow->getReportSection().getSectionView().MarkObj( *pObject, !_bMark );
            }
        }
    }
}
// -----------------------------------------------------------------------------
void OViewsWindow::collectRectangles(TRectangleMap& _rSortRectangles,  bool _bBoundRects)
{
    TSectionsMap::iterator aIter = m_aSections.begin();
	TSectionsMap::iterator aEnd = m_aSections.end();

    for (aIter = m_aSections.begin();aIter != aEnd ; ++aIter)
	{
        OSectionView& rView = (*aIter)->getReportSection().getSectionView();

		if(rView.areSdrObjectsSelected())
		{
			const SdrObjectVector aSelection(rView.getSelectedSdrObjectVectorFromSdrMarkView());
    
			for (sal_uInt32 i(0); i < aSelection.size(); ++i) 
        {
				SdrObject* pObj = aSelection[i];
				Rectangle aObjRect(_bBoundRects 
					? sdr::legacy::GetBoundRect(*pObj) 
					: sdr::legacy::GetSnapRect(*pObj));

                _rSortRectangles.insert(TRectangleMap::value_type(aObjRect,TRectangleMap::mapped_type(pObj,&rView)));
            }
        }
    }
}
// -----------------------------------------------------------------------------
void OViewsWindow::collectBoundResizeRect(const TRectangleMap& _rSortRectangles,sal_Int32 _nControlModification,bool _bAlignAtSection, bool _bBoundRects,Rectangle& _rBound,Rectangle& _rResize)
{
    bool bOnlyOnce = false;
    TRectangleMap::const_iterator aRectIter = _rSortRectangles.begin();
    TRectangleMap::const_iterator aRectEnd = _rSortRectangles.end();
	for (;aRectIter != aRectEnd ; ++aRectIter)
	{
        Rectangle aObjRect = aRectIter->first;
        if ( _rResize.IsEmpty() )
            _rResize = aObjRect;
        switch(_nControlModification)
        {
            case ControlModification::WIDTH_SMALLEST:
                if ( _rResize.getWidth() > aObjRect.getWidth() )
                    _rResize = aObjRect;
                break;
            case ControlModification::HEIGHT_SMALLEST:
                if ( _rResize.getHeight() > aObjRect.getHeight() )
                    _rResize = aObjRect;
                break;
            case ControlModification::WIDTH_GREATEST:
                if ( _rResize.getWidth() < aObjRect.getWidth() )
                    _rResize = aObjRect;
                break;
            case ControlModification::HEIGHT_GREATEST:
                if ( _rResize.getHeight() < aObjRect.getHeight() )
                    _rResize = aObjRect;
                break;
        }

        SdrObjTransformInfoRec aInfo;
        const SdrObject* pObj =  aRectIter->second.first;
		pObj->TakeObjInfo(aInfo);
        sal_Bool bHasFixed = !aInfo.mbMoveAllowed || pObj->IsMoveProtect();
		if ( bHasFixed ) 
			_rBound.Union(aObjRect);
        else
        {
		    if ( _bAlignAtSection || _rSortRectangles.size() == 1 )
            { // einzelnes Obj an der Seite ausrichten
                if ( ! bOnlyOnce )
                {
                    bOnlyOnce = true;
                    OReportSection* pReportSection = aRectIter->second.second->getReportSection();
		            const uno::Reference< report::XSection> xSection = pReportSection->getSection();
                    try
                    {
                        uno::Reference<report::XReportDefinition> xReportDefinition = xSection->getReportDefinition();
			            _rBound.Union(Rectangle(getStyleProperty<sal_Int32>(xReportDefinition,PROPERTY_LEFTMARGIN),0,
								            getStyleProperty<awt::Size>(xReportDefinition,PROPERTY_PAPERSIZE).Width  - getStyleProperty<sal_Int32>(xReportDefinition,PROPERTY_RIGHTMARGIN),
								            xSection->getHeight()));
                    }
                    catch(uno::Exception){}
                }
		    } 
            else 
            {
			    if (_bBoundRects) 
				{
					_rBound.Union(sdr::legacy::GetAllObjBoundRect(
						aRectIter->second.second->getSelectedSdrObjectVectorFromSdrMarkView()));
				}
			    else 
				{
                    const basegfx::B2DRange aSnapRange(aRectIter->second.second->getMarkedObjectSnapRange());

                    if(!aSnapRange.isEmpty())
                    {
			            _rBound.Union(
                            Rectangle(
					            (sal_Int32)floor(aSnapRange.getMinX()), (sal_Int32)floor(aSnapRange.getMinY()),
					            (sal_Int32)ceil(aSnapRange.getMaxX()), (sal_Int32)ceil(aSnapRange.getMaxY())));
                    }
				}
		    }
	    }
	}
}
// -----------------------------------------------------------------------------
void OViewsWindow::alignMarkedObjects(sal_Int32 _nControlModification,bool _bAlignAtSection, bool _bBoundRects)
{
    if ( _nControlModification == ControlModification::NONE )
        return;

    Point aRefPoint;
    RectangleLess::CompareMode eCompareMode = RectangleLess::POS_LEFT;
    switch (_nControlModification) 
    {
		case ControlModification::TOP   : eCompareMode = RectangleLess::POS_UPPER; break;
		case ControlModification::BOTTOM: eCompareMode = RectangleLess::POS_DOWN; break;
        case ControlModification::LEFT  : eCompareMode = RectangleLess::POS_LEFT; break;
		case ControlModification::RIGHT : eCompareMode = RectangleLess::POS_RIGHT; break;
        case ControlModification::CENTER_HORIZONTAL :
        case ControlModification::CENTER_VERTICAL :
            {
                eCompareMode = (ControlModification::CENTER_VERTICAL == _nControlModification) ?  RectangleLess::POS_CENTER_VERTICAL :  RectangleLess::POS_CENTER_HORIZONTAL; 
                uno::Reference<report::XSection> xSection = (*m_aSections.begin())->getReportSection().getSection();
                uno::Reference<report::XReportDefinition> xReportDefinition = xSection->getReportDefinition();
			    aRefPoint = Rectangle(getStyleProperty<sal_Int32>(xReportDefinition,PROPERTY_LEFTMARGIN),0,
								        getStyleProperty<awt::Size>(xReportDefinition,PROPERTY_PAPERSIZE).Width  - getStyleProperty<sal_Int32>(xReportDefinition,PROPERTY_RIGHTMARGIN),
								        xSection->getHeight()).Center();
            }
            break;
		default: break;
	}
    RectangleLess aCompare(eCompareMode,aRefPoint);
    TRectangleMap aSortRectangles(aCompare);
    collectRectangles(aSortRectangles,_bBoundRects);
    
    Rectangle aBound;
    Rectangle aResize;
    collectBoundResizeRect(aSortRectangles,_nControlModification,_bAlignAtSection,_bBoundRects,aBound,aResize);

    bool bMove = true;

    ::std::mem_fun_t<long&,Rectangle> aGetFun       = ::std::mem_fun<long&,Rectangle>(&Rectangle::Bottom);
    ::std::mem_fun_t<long&,Rectangle> aRefFun       = ::std::mem_fun<long&,Rectangle>(&Rectangle::Top);
    TRectangleMap::iterator aRectIter = aSortRectangles.begin();
    TRectangleMap::iterator aRectEnd = aSortRectangles.end();
	for (;aRectIter != aRectEnd ; ++aRectIter)
	{
        Rectangle aObjRect = aRectIter->first;
        SdrObject* pObj = aRectIter->second.first;
        SdrView* pView = aRectIter->second.second;
        Point aCenter(aBound.Center());
		SdrObjTransformInfoRec aInfo;
		pObj->TakeObjInfo(aInfo);
		if (aInfo.mbMoveAllowed && !pObj->IsMoveProtect()) 
        {
            long nXMov = 0;
            long nYMov = 0;
            long* pValue = &nXMov;
			switch(_nControlModification)
            {
				case ControlModification::TOP   : 
                    aGetFun  = ::std::mem_fun<long&,Rectangle>(&Rectangle::Top);
                    aRefFun  = ::std::mem_fun<long&,Rectangle>(&Rectangle::Bottom);
                    pValue = &nYMov;
                    break;
				case ControlModification::BOTTOM: 
                    // defaults are already set
                    pValue = &nYMov;
                    break;
				case ControlModification::CENTER_VERTICAL: 
                    nYMov = aCenter.Y() - aObjRect.Center().Y(); 
                    pValue = &nYMov;
                    bMove = false;
                    break;
				case ControlModification::RIGHT : 
                    aGetFun  = ::std::mem_fun<long&,Rectangle>(&Rectangle::Right);
                    aRefFun  = ::std::mem_fun<long&,Rectangle>(&Rectangle::Left);
                    break;
				case ControlModification::CENTER_HORIZONTAL: 
                    nXMov = aCenter.X() - aObjRect.Center().X();
                    bMove = false;
                    break;
                case ControlModification::LEFT  : 
                    aGetFun  = ::std::mem_fun<long&,Rectangle>(&Rectangle::Left);
                    aRefFun  = ::std::mem_fun<long&,Rectangle>(&Rectangle::Right);
                    break;
                default:
                    bMove = false;
                    break;
			}
            if ( bMove )
            {
                Rectangle aTest = aObjRect;
                aGetFun(&aTest) = aGetFun(&aBound);
                TRectangleMap::iterator aInterSectRectIter = aSortRectangles.begin();
                for (; aInterSectRectIter != aRectIter; ++aInterSectRectIter)
                {
                    if ( pView == aInterSectRectIter->second.second && (dynamic_cast<OUnoObject*>(aInterSectRectIter->second.first) || dynamic_cast<OOle2Obj*>(aInterSectRectIter->second.first)))
                    {
                        SdrObject* pPreviousObj = aInterSectRectIter->second.first;
                        Rectangle aIntersectRect(aTest.GetIntersection(_bBoundRects 
							? sdr::legacy::GetBoundRect(*pPreviousObj) 
							: sdr::legacy::GetSnapRect(*pPreviousObj)));
                        if ( !aIntersectRect.IsEmpty() && (aIntersectRect.Left() != aIntersectRect.Right() && aIntersectRect.Top() != aIntersectRect.Bottom() ) )
                        {
                            *pValue = aRefFun(&aIntersectRect) - aGetFun(&aObjRect);
                            break;
                        }
                    }
                }
                if ( aInterSectRectIter == aRectIter )
                    *pValue = aGetFun(&aBound) - aGetFun(&aObjRect);
            }
            
            if ( lcl_getNewRectSize(aObjRect,nXMov,nYMov,pObj,pView,_nControlModification,_bBoundRects) )
            {
                pView->AddUndo(pView->getSdrModelFromSdrView().GetSdrUndoFactory().CreateUndoGeoObject(*pObj));
                sdr::legacy::transformSdrObject(*pObj, basegfx::tools::createTranslateB2DHomMatrix(nXMov, nYMov));
                aObjRect = (_bBoundRects 
					? sdr::legacy::GetBoundRect(*pObj) 
					: sdr::legacy::GetSnapRect(*pObj));
            }

            // resizing control
            if ( !aResize.IsEmpty() && aObjRect != aResize )
            {
                nXMov = aResize.getWidth();
                nYMov = aResize.getHeight();
                switch(_nControlModification)
                {
                    case ControlModification::WIDTH_GREATEST:
                    case ControlModification::HEIGHT_GREATEST:
                        if ( _nControlModification == ControlModification::HEIGHT_GREATEST )
                            nXMov = aObjRect.getWidth();
                        else if ( _nControlModification == ControlModification::WIDTH_GREATEST )
                            nYMov = aObjRect.getHeight();
                        lcl_getNewRectSize(aObjRect,nXMov,nYMov,pObj,pView,_nControlModification,_bBoundRects);
                        // run through
                    case ControlModification::WIDTH_SMALLEST:
                    case ControlModification::HEIGHT_SMALLEST:
                        pView->AddUndo( pView->getSdrModelFromSdrView().GetSdrUndoFactory().CreateUndoGeoObject(*pObj));
                        {
                            OObjectBase* pObjBase = dynamic_cast<OObjectBase*>(pObj);
                            OSL_ENSURE(pObjBase,"Where comes this object from?");
                            if ( pObjBase )
                            {
                                if ( _nControlModification == ControlModification::WIDTH_SMALLEST || _nControlModification == ControlModification::WIDTH_GREATEST )
                                    pObjBase->getReportComponent()->setSize(awt::Size(nXMov,aObjRect.getHeight()));
                                    //pObj->Resize(aObjRect.TopLeft(),Fraction(nXMov,aObjRect.getWidth()),Fraction(1,1));
                                else if ( _nControlModification == ControlModification::HEIGHT_GREATEST || _nControlModification == ControlModification::HEIGHT_SMALLEST )
                                    pObjBase->getReportComponent()->setSize(awt::Size(aObjRect.getWidth(),nYMov));
                                    //pObj->Resize(aObjRect.TopLeft(),Fraction(1,1),Fraction(nYMov,aObjRect.getHeight()));
                            }
                        }
                        break;
				    default: 
                        break;
                }
            }
		}

        pView->SetMarkHandles();
    }
}
// -----------------------------------------------------------------------------
void OViewsWindow::createDefault()
{
    ::boost::shared_ptr<OSectionWindow> pMarkedSection = getMarkedSection();
    if ( pMarkedSection )
		pMarkedSection->getReportSection().createDefault(m_sShapeType);
}
// -----------------------------------------------------------------------------
void OViewsWindow::setGridSnap(sal_Bool bOn)
{
    TSectionsMap::iterator aIter = m_aSections.begin();
	TSectionsMap::iterator aEnd = m_aSections.end();
	for (; aIter != aEnd ; ++aIter)
    {
		(*aIter)->getReportSection().getSectionView().SetGridSnap(bOn);
        static sal_Int32 nIn = 0;
        (*aIter)->getReportSection().Invalidate(nIn);
    }
}
// -----------------------------------------------------------------------------
void OViewsWindow::setDragStripes(sal_Bool bOn)
{
    TSectionsMap::iterator aIter = m_aSections.begin();
	TSectionsMap::iterator aEnd = m_aSections.end();
	for (; aIter != aEnd ; ++aIter)
		(*aIter)->getReportSection().getSectionView().SetDragStripes(bOn);
}
// -----------------------------------------------------------------------------
sal_uInt16 OViewsWindow::getPosition(const OSectionWindow* _pSectionWindow) const
{
	TSectionsMap::const_iterator aIter = m_aSections.begin();
	TSectionsMap::const_iterator aEnd = m_aSections.end();
	sal_uInt16 nPosition = 0;
	for (; aIter != aEnd ; ++aIter)
	{
		if ( _pSectionWindow == (*aIter).get() )
		{
			break;
		}
		++nPosition;
	}
	return nPosition;
}
// -----------------------------------------------------------------------------
::boost::shared_ptr<OSectionWindow> OViewsWindow::getSectionWindow(const sal_uInt16 _nPos) const
{
    ::boost::shared_ptr<OSectionWindow> aReturn;
    
    if ( _nPos < m_aSections.size() )
        aReturn = m_aSections[_nPos];

    return aReturn;
}
// -----------------------------------------------------------------------------
namespace
{
    enum SectionViewAction
    {
        eEndDragObj,
        eEndAction,
        eMoveAction,
        eMarkAction,
        eBreakAction
    };
    class ApplySectionViewAction : public ::std::unary_function< OViewsWindow::TSectionsMap::value_type, void >
    {
    private:
        SectionViewAction   m_eAction;
        sal_Bool            m_bCopy;
        basegfx::B2DPoint	m_aPoint;

    public:
        ApplySectionViewAction( sal_Bool _bCopy ) : m_eAction( eEndDragObj ), m_bCopy( _bCopy ) { }
        ApplySectionViewAction(SectionViewAction _eAction = eEndAction ) : m_eAction( _eAction ) { }
        ApplySectionViewAction(const basegfx::B2DPoint& _rPoint, SectionViewAction _eAction = eMoveAction ) : m_eAction( _eAction ), m_bCopy( sal_False ), m_aPoint( _rPoint ) { }

        void operator() ( const OViewsWindow::TSectionsMap::value_type& _rhs )
        {
            OSectionView& rView( _rhs->getReportSection().getSectionView() );
            switch ( m_eAction )
            {
            case eEndDragObj: 
                rView.EndDragObj( m_bCopy  );
                break;
            case eEndAction:
                if ( rView.IsAction() ) 
                    rView.EndAction (      ); 
                break;
            case eMoveAction:
                rView.MovAction ( m_aPoint );
                break;
            case eMarkAction:
                rView.BegMarkObj ( m_aPoint );
                break;
            case eBreakAction:
                if ( rView.IsAction() ) 
                    rView.BrkAction (      ); 
                break;
                // default:
                
            }
        }
    };
}
// -----------------------------------------------------------------------------
void OViewsWindow::BrkAction()
{
	EndDragObj_removeInvisibleObjects();
    ::std::for_each( m_aSections.begin(), m_aSections.end(), ApplySectionViewAction(eBreakAction) );
}
// -----------------------------------------------------------------------------
void OViewsWindow::BegDragObj_createInvisibleObjectAtPosition(const basegfx::B2DRange& _aRange, const OSectionView& _rSection)
{
    TSectionsMap::iterator aIter = m_aSections.begin();
    TSectionsMap::iterator aEnd = m_aSections.end();
    Point aNewPos(0,0);

    for (; aIter != aEnd; ++aIter)
    {
        OReportSection& rReportSection = (*aIter)->getReportSection();
		rReportSection.getPage()->setSpecialMode();
		OSectionView& rView = rReportSection.getSectionView();
        
        if ( &rView != &_rSection )
        {
			SdrObject *pNewObj = new SdrUnoObj(
				rView.getSdrModelFromSdrView(),
				::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("com.sun.star.form.component.FixedText")));

			if (pNewObj)
			{
                sdr::legacy::SetLogicRange(*pNewObj, _aRange);
				sdr::legacy::MoveSdrObject(*pNewObj, Size(0, aNewPos.Y()));
                sal_Bool bChanged = rView.getSdrModelFromSdrView().IsChanged();
	            rReportSection.getPage()->InsertObjectToSdrObjList(*pNewObj);
                rView.getSdrModelFromSdrView().SetChanged(bChanged);
                m_aBegDragTempList.push_back(pNewObj);
                const Rectangle aRect(sdr::legacy::GetLogicRect(*pNewObj));

				// pNewObj->SetText(String::CreateFromAscii("Drag helper"));
                rView.MarkObj( *pNewObj );
			}
		}
	    const long nSectionHeight = rReportSection.PixelToLogic(rReportSection.GetOutputSizePixel()).Height();
        aNewPos.Y() -= nSectionHeight;
//        aNewPos.Y() -= PixelToLogic(aIter->second.second->GetSizePixel()).Height();
    }
}
// -----------------------------------------------------------------------------
bool OViewsWindow::isObjectInMyTempList(SdrObject *_pObj)
{
    return ::std::find(m_aBegDragTempList.begin(),m_aBegDragTempList.end(),_pObj) != m_aBegDragTempList.end();
}

// -----------------------------------------------------------------------------
void OViewsWindow::BegDragObj(const basegfx::B2DPoint& _aPnt, const SdrHdl* _pHdl, const OSectionView* _pSection)
{   	
	OSL_TRACE("BegDragObj Clickpoint X:%f Y:%f\n", _aPnt.getX(), _aPnt.getY() );

    m_aBegDragTempList.clear();

	// Calculate the absolute clickpoint in the views
	basegfx::B2DPoint aAbsolutePnt(_aPnt);
	TSectionsMap::iterator aIter = m_aSections.begin();
    TSectionsMap::iterator aEnd = m_aSections.end();
	for (; aIter != aEnd; ++aIter)
    {
        OReportSection& rReportSection = (*aIter)->getReportSection();   
        OSectionView* pView = &rReportSection.getSectionView();
		if (pView == _pSection)
			break;
        const long nSectionHeight = rReportSection.PixelToLogic(rReportSection.GetOutputSizePixel()).Height();
		aAbsolutePnt.setY(aAbsolutePnt.getY() + nSectionHeight);
	}
	m_aDragDelta = basegfx::B2DPoint(SAL_MAX_INT32, SAL_MAX_INT32);
	OSL_TRACE("BegDragObj Absolute X:%f Y:%f\n", aAbsolutePnt.getX(), aAbsolutePnt.getY() );

    // Create drag lines over all viewable Views
	// Therefore we need to identify the marked objects
	// and create temporary objects on all other views at the same position
	// relative to its occurance.

    OSL_TRACE("BegDragObj createInvisible Objects\n" );
    int nViewCount = 0;
	basegfx::B2DPoint aNewObjPos(0.0, 0.0);
    basegfx::B2DPoint aLeftTop(SAL_MAX_INT32, SAL_MAX_INT32);
    for (aIter = m_aSections.begin(); aIter != aEnd; ++aIter)
    {
        OReportSection& rReportSection = (*aIter)->getReportSection();
        OSectionView& rView = rReportSection.getSectionView();
		const SdrObjectVector aSelection(rView.getSelectedSdrObjectVectorFromSdrMarkView());
        
		for(sal_uInt32 i(0); i < aSelection.size(); ++i) 
            {
		    SdrObject* pObj = aSelection[i];
            
                if (!isObjectInMyTempList(pObj))
                {
				basegfx::B2DRange aRange(pObj->getObjectRange(&rView));
				aRange.transform(basegfx::tools::createTranslateB2DHomMatrix(0.0, aNewObjPos.getY()));
                aLeftTop.setX(::std::min( aRange.getMinX(), aLeftTop.getX() ));
                aLeftTop.setY(::std::min( aRange.getMinY(), aLeftTop.getY() ));

                OSL_TRACE("BegDragObj createInvisible X:%g Y:%g on View #%d\n", aRange.getMinX(), aRange.getMinY(), nViewCount );
                    
                BegDragObj_createInvisibleObjectAtPosition(aRange, rView);
                    
                    // calculate the clickpoint 
//                    const sal_Int32 nDeltaX = abs(aRect.Left() - aAbsolutePnt.X());
//                    const sal_Int32 nDeltaY = abs(aRect.Top() - aAbsolutePnt.Y());
//                    if (m_aDragDelta.X() > nDeltaX)
//                        m_aDragDelta.X() = nDeltaX;
//                    if (m_aDragDelta.Y() > nDeltaY)
//                        m_aDragDelta.Y() = nDeltaY;
                }
            } 
        ++nViewCount;

		basegfx::B2DRange aWorkArea(rView.GetWorkArea());
		aWorkArea = basegfx::B2DRange(aWorkArea.getMinX(), -aNewObjPos.getY(), aWorkArea.getMaxX(), aWorkArea.getMaxY());
        rView.SetWorkArea(aWorkArea);

        const long nSectionHeight = rReportSection.PixelToLogic(rReportSection.GetOutputSizePixel()).Height();
	    aNewObjPos.setY(aNewObjPos.getY() + nSectionHeight);

        // don't subtract the height of the lines between the views
        // aNewObjPos.Y() -= PixelToLogic(aIter->second.second->GetSizePixel()).Height();
    }
    
	m_aDragDelta = absolute(aLeftTop - aAbsolutePnt);

    basegfx::B2DPoint aNewPos(aAbsolutePnt);
    // for (aIter = m_aSections.begin(); aIter != aEnd; ++aIter)
    // {
    //     OReportSection& rReportSection = (*aIter)->getReportSection();
    //     if ( &rReportSection.getSectionView() == _pSection )
    //         break;
    //     aNewPos.Y() += rReportSection.PixelToLogic(rReportSection.GetOutputSizePixel()).Height();
    // }

	// long nLastSectionHeight = 0;
    // bool bAdd = true;
    nViewCount = 0;
    for (aIter = m_aSections.begin(); aIter != aEnd; ++aIter)
    {
        OReportSection& rReportSection = (*aIter)->getReportSection();

        // if ( &rReportSection.getSectionView() == _pSection )
        // {
        //     bAdd = false;
        //     aNewPos = _aPnt;
        // }
        // else if ( bAdd )
        // {
	    //     const long nSectionHeight = rReportSection.PixelToLogic(rReportSection.GetOutputSizePixel()).Height();
        //     aNewPos.Y() += nSectionHeight;
        // }
        // else
        // {
        //     aNewPos.Y() -= nLastSectionHeight;
        // }

        //?
        const SdrHdl* pHdl = _pHdl;
        if ( pHdl )
        {
            if ( &rReportSection.getSectionView() != _pSection )
            {
                const SdrHdlList& rHdlList = rReportSection.getSectionView().GetHdlList();
                pHdl = rHdlList.GetHdlByKind(_pHdl->GetKind());
            }
        }
        OSL_TRACE("BegDragObj X:%f Y:%f on View#%d\n", aNewPos.getX(), aNewPos.getY(), nViewCount++ );
		const double fTolerance(basegfx::B2DVector(GetInverseViewTransformation() * basegfx::B2DVector(3.0, 0.0)).getLength());
        rReportSection.getSectionView().BegDragObj(aNewPos, pHdl, fTolerance, NULL);

        const long nSectionHeight = rReportSection.PixelToLogic(rReportSection.GetOutputSizePixel()).Height();
        aNewPos.setY(aNewPos.getY() - nSectionHeight);
        // subtract the height between the views, because they are visible but not from interest here.
        // aNewPos.Y() -= PixelToLogic(aIter->second.second->GetSizePixel()).Height();
    }
}

// -----------------------------------------------------------------------------
void OViewsWindow::BegMarkObj(const basegfx::B2DPoint& _aPnt,const OSectionView* _pSection)
{
    bool bAdd = true;
    basegfx::B2DPoint aNewPos(_aPnt);

    TSectionsMap::iterator aIter = m_aSections.begin();
    TSectionsMap::iterator aEnd = m_aSections.end();
    double fLastSectionHeight(0.0);

    for (; aIter != aEnd; ++aIter)
    {
        OReportSection& rReportSection = (*aIter)->getReportSection();
        if ( &rReportSection.getSectionView() == _pSection )
        {
            bAdd = false;
            aNewPos = _aPnt; // 2,2 
        }
        else if ( bAdd )
        {
	        const double fSectionHeight(basegfx::B2DVector(rReportSection.GetInverseViewTransformation() *
				basegfx::B2DVector(rReportSection.GetOutputSizePixel().getWidth(), 0.0)).getLength());
            aNewPos.setY(aNewPos.getY() + fSectionHeight);
        }
        else
        {
            aNewPos.setY(aNewPos.getY() - fLastSectionHeight);
        }

        rReportSection.getSectionView().BegMarkObj ( aNewPos );
	    fLastSectionHeight = basegfx::B2DVector(rReportSection.GetInverseViewTransformation() *
			basegfx::B2DVector(rReportSection.GetOutputSizePixel().getWidth(), 0.0)).getLength();

        // aNewPos.Y() -= PixelToLogic(aIter->second.second->GetSizePixel()).Height();
    }
    //::std::for_each( m_aSections.begin(), m_aSections.end(), ApplySectionViewAction( _aPnt , eMarkAction) );
}
// -----------------------------------------------------------------------------
OSectionView* OViewsWindow::getSectionRelativeToPosition(const OSectionView* _pSection,basegfx::B2DPoint& _rPnt)
{
    OSectionView* pSection = NULL;
    sal_Int32 nCount = 0;
    TSectionsMap::iterator aIter = m_aSections.begin();
    const TSectionsMap::iterator aEnd = m_aSections.end();
    for (; aIter != aEnd ; ++aIter,++nCount)
    {
        OReportSection& rReportSection = (*aIter)->getReportSection();
        if ( &rReportSection.getSectionView() == _pSection)
            break;
    }
    OSL_ENSURE(aIter != aEnd,"This can never happen!");
    if ( _rPnt.getY() < 0.0 )
    {
        if ( nCount )
            --aIter;
        for (; nCount && (_rPnt.getY() < 0.0); --nCount)
        {
            OReportSection& rReportSection = (*aIter)->getReportSection();
            const sal_Int32 nHeight = rReportSection.PixelToLogic(rReportSection.GetOutputSizePixel()).Height();
            _rPnt.setY(_rPnt.getY() + nHeight);
            if ( (nCount -1) > 0 && (_rPnt.getY() < 0.0) )
                --aIter;
        }
        if ( nCount == 0 )
            pSection = &(*m_aSections.begin())->getReportSection().getSectionView();
        else
            pSection = &(*aIter)->getReportSection().getSectionView();
    }
    else
    {
        for (; aIter != aEnd; ++aIter)
        {
            OReportSection& rReportSection = (*aIter)->getReportSection();
            const long nHeight = rReportSection.PixelToLogic(rReportSection.GetOutputSizePixel()).Height();
            if ( (_rPnt.getY() - nHeight) < 0.0  )
                break;
            _rPnt.setY(_rPnt.getY() - nHeight);
        }
        if ( aIter != aEnd )
            pSection = &(*aIter)->getReportSection().getSectionView();
        else
            pSection = &(*(aEnd-1))->getReportSection().getSectionView();
    }

    return pSection;
}
// -----------------------------------------------------------------------------
void OViewsWindow::EndDragObj_removeInvisibleObjects()
{
    TSectionsMap::iterator aIter = m_aSections.begin();
    TSectionsMap::iterator aEnd = m_aSections.end();

    for (; aIter != aEnd; ++aIter)
    {
        OReportSection& rReportSection = (*aIter)->getReportSection();
		rReportSection.getPage()->resetSpecialMode();
	}
}
// -----------------------------------------------------------------------------
void OViewsWindow::EndDragObj(sal_Bool _bControlKeyPressed, const OSectionView* _pSection,const basegfx::B2DPoint& _aPnt)
{
	const String sUndoAction = String((ModuleRes(RID_STR_UNDO_CHANGEPOSITION)));
    const UndoContext aUndoContext( getView()->getReportView()->getController().getUndoManager(), sUndoAction );

    basegfx::B2DPoint aNewPos(_aPnt);
    OSectionView* pInSection = getSectionRelativeToPosition(_pSection, aNewPos);
	if (!_bControlKeyPressed &&
        (_pSection && ( _pSection->IsDragResize() == false ) ) && /* Not in resize mode */
        _pSection != pInSection)
    {
        EndDragObj_removeInvisibleObjects();

		// we need to manipulate the current clickpoint, we substract the old delta from BeginDrag
        // OSectionView* pInSection = getSectionRelativeToPosition(_pSection, aPnt);
        // aNewPos.X() -= m_aDragDelta.X();
        // aNewPos.Y() -= m_aDragDelta.Y();
	    aNewPos -= m_aDragDelta;

        uno::Sequence< beans::NamedValue > aAllreadyCopiedObjects;
        TSectionsMap::iterator aIter = m_aSections.begin();
        const TSectionsMap::iterator aEnd = m_aSections.end();
        for (; aIter != aEnd; ++aIter)
        {
            OReportSection& rReportSection = (*aIter)->getReportSection();
            if ( pInSection != &rReportSection.getSectionView() )
            {
                rReportSection.getSectionView().BrkAction();
                rReportSection.Copy(aAllreadyCopiedObjects,true);
            }
            else
                pInSection->EndDragObj(sal_False);
        } // for (; aIter != aEnd; ++aIter)

        if ( aAllreadyCopiedObjects.getLength() )
        {
            beans::NamedValue* pIter = aAllreadyCopiedObjects.getArray();
            const beans::NamedValue* pEnd = pIter + aAllreadyCopiedObjects.getLength();
            try
            {
                uno::Reference<report::XReportDefinition> xReportDefinition = getView()->getReportView()->getController().getReportDefinition();
	            const sal_Int32 nLeftMargin  = getStyleProperty<sal_Int32>(xReportDefinition,PROPERTY_LEFTMARGIN);
	            const sal_Int32 nRightMargin = getStyleProperty<sal_Int32>(xReportDefinition,PROPERTY_RIGHTMARGIN);
	            const sal_Int32 nPaperWidth  = getStyleProperty<awt::Size>(xReportDefinition,PROPERTY_PAPERSIZE).Width;

                if ( aNewPos.getX() < nLeftMargin )
                    aNewPos.setX(nLeftMargin);
                if ( aNewPos.getY() < 0.0 )
                    aNewPos.setY(0.0);
                
                basegfx::B2DPoint aPrevious;
                for (; pIter != pEnd; ++pIter)
                {
                    uno::Sequence< uno::Reference<report::XReportComponent> > aClones;
                    pIter->Value >>= aClones;
                    uno::Reference<report::XReportComponent>* pColIter = aClones.getArray();
                    const uno::Reference<report::XReportComponent>* pColEnd = pColIter + aClones.getLength();

                    // move the cloned Components to new positions
                    for (; pColIter != pColEnd; ++pColIter)
                    {
                        uno::Reference< report::XReportComponent> xRC(*pColIter);
                        aPrevious = basegfx::B2DPoint(xRC->getPosition().X, xRC->getPosition().Y);
                        awt::Size aSize = xRC->getSize();

                        if ( aNewPos.getX() < nLeftMargin )
                        {
                            aNewPos.setX(nLeftMargin);
                        }
                        else if ( (aNewPos.getX() + aSize.Width) > (nPaperWidth - nRightMargin) )
                        {
                            aNewPos.setX(nPaperWidth - nRightMargin - aSize.Width);
                        }
                        if ( aNewPos.getY() < 0.0 )
                        {
                            aNewPos.setY(0.0);
                        }
                        if ( aNewPos.getX() < 0.0 )
                        {
                            aSize.Width += basegfx::fround(aNewPos.getX());
                            aNewPos.setX(0.0);
                            xRC->setSize(aSize);
                        }
                        xRC->setPosition(::com::sun::star::awt::Point(basegfx::fround(aNewPos.getX()), basegfx::fround(aNewPos.getY())));
                        if ( (pColIter+1) != pColEnd )
                        {
                            // bring aNewPos to the position of the next object
                            uno::Reference< report::XReportComponent> xRCNext(*(pColIter + 1),uno::UNO_QUERY);
						    const basegfx::B2DPoint aNextPosition(xRCNext->getPosition().X, xRCNext->getPosition().Y);
						    aNewPos += aNextPosition - aPrevious;
                        }
                    }
                }
            }
            catch(uno::Exception&)
            {
            }
            pInSection->getReportSection()->Paste(aAllreadyCopiedObjects,true);
        }
    }
    else
	{
		::std::for_each( m_aSections.begin(), m_aSections.end(), ApplySectionViewAction( sal_False ) );
        EndDragObj_removeInvisibleObjects();
	}
    m_aDragDelta = basegfx::B2DPoint(SAL_MAX_INT32, SAL_MAX_INT32);
}
// -----------------------------------------------------------------------------
void OViewsWindow::EndAction()
{
    ::std::for_each( m_aSections.begin(), m_aSections.end(), ApplySectionViewAction() );
}
// -----------------------------------------------------------------------------
void OViewsWindow::MovAction(const basegfx::B2DPoint& _aPnt,const OSectionView* _pSection,bool _bMove, bool _bControlKeySet)
{
	(void)_bMove;

	basegfx::B2DPoint aRealMousePos(_aPnt);
	basegfx::B2DPoint aCurrentSectionPos(0.0, 0.0);
    OSL_TRACE("MovAction X:%g Y:%g\n", aRealMousePos.getX(), aRealMousePos.getY() );

    basegfx::B2DPoint aHdlPos;
    SdrHdl* pHdl = _pSection->GetDragHdl();
    if ( pHdl )
    {
        aHdlPos = pHdl->getPosition();
    }

    TSectionsMap::iterator aIter/*  = m_aSections.begin() */;
    TSectionsMap::iterator aEnd = m_aSections.end();

	//if ( _bMove )
    //{
    for (aIter = m_aSections.begin(); aIter != aEnd; ++aIter)
    {
        OReportSection& rReportSection = (*aIter)->getReportSection();
        if ( &rReportSection.getSectionView() == _pSection )
            break;
        const long nSectionHeight = (*aIter)->PixelToLogic(rReportSection.GetOutputSizePixel()).Height();
        aCurrentSectionPos.setY(aCurrentSectionPos.getY() + nSectionHeight);
    } // for (aIter = m_aSections.begin(); aIter != aEnd; ++aIter)
	//}
	aRealMousePos += aCurrentSectionPos;

    // If control key is pressed the work area is limited to the section with the current selection.
	basegfx::B2DPoint aPosForWorkArea(0.0, 0.0);
    for (aIter = m_aSections.begin(); aIter != aEnd; ++aIter)
	{
		OReportSection& rReportSection = (*aIter)->getReportSection();
        OSectionView& rView = rReportSection.getSectionView();
		const long nSectionHeight = (*aIter)->PixelToLogic((*aIter)->GetOutputSizePixel()).Height();
		basegfx::B2DPoint aClipTopLeft(rView.GetWorkArea().getMinimum());
		basegfx::B2DPoint aClipBottomRight(rView.GetWorkArea().getMaximum());

		if (_bControlKeySet)
		{
			aClipTopLeft.setY(aCurrentSectionPos.getY() - aPosForWorkArea.getY());
			aClipBottomRight.setY(aClipTopLeft.getY() + nSectionHeight);
		}
		else
		{
			aClipTopLeft.setY(aClipTopLeft.getY() - aPosForWorkArea.getY());
    }
	
		rView.SetWorkArea(basegfx::B2DRange(aClipTopLeft, aClipBottomRight));
        aPosForWorkArea.setY(aPosForWorkArea.getY() + nSectionHeight);
    }

    for (aIter = m_aSections.begin(); aIter != aEnd; ++aIter)
    {
        OReportSection& rReportSection = (*aIter)->getReportSection();
        SdrHdl* pCurrentHdl = rReportSection.getSectionView().GetDragHdl();
        if ( pCurrentHdl )
        {
			if ( aRealMousePos.getY() > 0.0 )
	            aRealMousePos = _aPnt + pCurrentHdl->getPosition() - aHdlPos;
        }
        rReportSection.getSectionView().MovAction ( aRealMousePos );
        const long nSectionHeight = (*aIter)->PixelToLogic((*aIter)->GetOutputSizePixel()).Height();
        aRealMousePos.setY(aRealMousePos.getY() - nSectionHeight);
    }
}
// -----------------------------------------------------------------------------
sal_Bool OViewsWindow::IsAction() const
{
    sal_Bool bAction = sal_False;
    TSectionsMap::const_iterator aIter = m_aSections.begin();
    TSectionsMap::const_iterator aEnd = m_aSections.end();
    for (; !bAction && aIter != aEnd; ++aIter)
        bAction = (*aIter)->getReportSection().getSectionView().IsAction();
    return bAction;
}
// -----------------------------------------------------------------------------
sal_Bool OViewsWindow::IsDragObj() const
{
    sal_Bool bAction = sal_False;
    TSectionsMap::const_iterator aIter = m_aSections.begin();
    TSectionsMap::const_iterator aEnd = m_aSections.end();
    for (; !bAction && aIter != aEnd; ++aIter)
        bAction = (*aIter)->getReportSection().getSectionView().IsAction();
    return bAction;
}
// -----------------------------------------------------------------------------
sal_uInt32 OViewsWindow::getMarkedObjectCount() const
{
    sal_uInt32 nCount = 0;
    TSectionsMap::const_iterator aIter = m_aSections.begin();
    TSectionsMap::const_iterator aEnd = m_aSections.end();
    for (; aIter != aEnd; ++aIter)
        nCount += (*aIter)->getReportSection().getSectionView().getSelectedSdrObjectCount();
    return nCount;
}
// -----------------------------------------------------------------------------
void OViewsWindow::handleKey(const KeyCode& _rCode)
{
    const sal_uInt16 nCode = _rCode.GetCode();
    if ( _rCode.IsMod1() )
    {
        // scroll page
		OScrollWindowHelper* pScrollWindow = getView()->getScrollWindow();
		ScrollBar* pScrollBar = ( nCode == KEY_LEFT || nCode == KEY_RIGHT ) ? pScrollWindow->GetHScroll() : pScrollWindow->GetVScroll();
        if ( pScrollBar && pScrollBar->IsVisible() )
			pScrollBar->DoScrollAction(( nCode == KEY_RIGHT || nCode == KEY_UP ) ? SCROLL_LINEUP : SCROLL_LINEDOWN );
        return;
    }
    TSectionsMap::const_iterator aIter = m_aSections.begin();
    TSectionsMap::const_iterator aEnd = m_aSections.end();
    for (; aIter != aEnd; ++aIter)
    {
        OReportSection& rReportSection = (*aIter)->getReportSection();
		basegfx::B2DVector aMove(0.0, 0.0);

	    if ( nCode == KEY_UP )
		    aMove.setY(-1.0);
	    else if ( nCode == KEY_DOWN )
		    aMove.setY(1.0);
	    else if ( nCode == KEY_LEFT )
		    aMove.setX(-1.0);
	    else if ( nCode == KEY_RIGHT )
		    aMove.setX(1.0);

	    if ( rReportSection.getSectionView().areSdrObjectsSelected() )
	    {
		    if ( _rCode.IsMod2() )
		    {
			    // move in 1 pixel distance
			    const Size aPixelSize = rReportSection.PixelToLogic( Size( 1, 1 ) );
				aMove *= basegfx::B2DVector(aPixelSize.getWidth(), aPixelSize.getHeight());
		    }
		    else
		    {
			    // move in 1 mm distance
				aMove *= DEFAUL_MOVE_SIZE;
		    }

            OSectionView& rView = rReportSection.getSectionView();
		    const SdrHdlList& rHdlList = rView.GetHdlList();
		    SdrHdl* pHdl = rHdlList.GetFocusHdl();

		    if ( pHdl == 0 )
		    {
			    // no handle selected
			    if ( rView.IsMoveAllowed() )
			    {
				    // restrict movement to work area
				    basegfx::B2DRange aWorkArea(rView.GetWorkArea());

				    if ( !aWorkArea.isEmpty() )
				    {
                        if ( aWorkArea.getMinY() < 0.0 )
				    {
							aWorkArea = basegfx::B2DRange(aWorkArea.getMinX(), 0.0, aWorkArea.getMaxX(), aWorkArea.getMaxY());
						}
					    
						basegfx::B2DRange aMarkRange( rView.getMarkedObjectSnapRange() );
					    aMarkRange.transform(basegfx::tools::createTranslateB2DHomMatrix(aMove));

					    if ( !aWorkArea.isInside( aMarkRange ) )
					    {
						    if ( aMarkRange.getMinX() < aWorkArea.getMinX() )
							{
							    aMove.setX(aMove.getX() + aWorkArea.getMinX() - aMarkRange.getMinX());
							}

						    if ( aMarkRange.getMaxX() > aWorkArea.getMaxX() )
							{
							    aMove.setX(aMove.getX() - aMarkRange.getMaxX() - aWorkArea.getMaxX());
							}

						    if ( aMarkRange.getMinY() < aWorkArea.getMinY() )
							{
							    aMove.setY(aMove.getY() + aWorkArea.getMinY() - aMarkRange.getMinY());
							}

						    if ( aMarkRange.getMaxY() > aWorkArea.getMaxY() )
							{
							    aMove.setY(aMove.getY() - aMarkRange.getMaxY() - aWorkArea.getMaxY());
					    }
					    }
                        
                        bool bCheck = false;
						const SdrObjectVector aSelection(rView.getSelectedSdrObjectVectorFromSdrMarkView());
                        
						for (sal_uInt32 i(0); !bCheck && i < aSelection.size(); ++i )
                        {
                            bCheck = dynamic_cast< OUnoObject* >(aSelection[i]) != NULL
								|| dynamic_cast< OOle2Obj* >(aSelection[i]) != NULL;
                        }
                        
                        if ( bCheck )
                        {
                            SdrObject* pOverlapped = isOver(aMarkRange,*rReportSection.getPage(),rView);
                            if ( pOverlapped )
                            {
                                do
                                {
                                    const basegfx::B2DRange& rOver = pOverlapped->getObjectRange(&rView);
                                    basegfx::B2DPoint aPos(0.0, 0.0);
                                    if ( nCode == KEY_UP )
                                    {
                                        aPos.setX(aMarkRange.getMinX());
                                        aPos.setY(rOver.getMinY() - aMarkRange.getHeight());
                                        aMove.setY(aMove.getY() + (aPos.getY() - aMarkRange.getMinY()));
                                    }
	                                else if ( nCode == KEY_DOWN )
                                    {
                                        aPos.setX(aMarkRange.getMinX());
                                        aPos.setY(rOver.getMaxY());
                                        aMove.setY(aMove.getY() + (aPos.getY() - aMarkRange.getMinY()));
                                    }
	                                else if ( nCode == KEY_LEFT )
                                    {
                                        aPos.setX(rOver.getMinX() - aMarkRange.getWidth());
                                        aPos.setY(aMarkRange.getMinY());
                                        aMove.setX(aMove.getX() + (aPos.getX() - aMarkRange.getMinX()));
                                    }
	                                else if ( nCode == KEY_RIGHT )
                                    {
                                        aPos.setX(rOver.getMaxX());
                                        aPos.setY(aMarkRange.getMinY());
                                        aMove.setX(aMove.getX() + (aPos.getX() - aMarkRange.getMinX()));
                                    }

									aMarkRange = basegfx::B2DRange(
										aPos.getX(),
										aPos.getY(),
										aPos.getX() + aMarkRange.getWidth(),
										aPos.getY() + aMarkRange.getHeight());
                                    
									if ( !aWorkArea.isInside( aMarkRange ) )
					                {
                                        break;
                                    }

									pOverlapped = isOver(aMarkRange,*rReportSection.getPage(),rView);
                                }
                                while(pOverlapped != NULL);
                                if (pOverlapped != NULL)
                                    break;
                            }
                        }
				    }

				    if ( !aMove.equalZero() )
				    {
					    rView.MoveMarkedObj(aMove);
					    rView.MakeVisibleAtView( rView.getMarkedObjectSnapRange(), rReportSection);
				    }
			    }
		    }
		    else
		    {
			    // move the handle
			    if ( pHdl && !aMove.equalZero() )
			    {
				    const basegfx::B2DPoint aStartPoint( pHdl->getPosition() );
				    const basegfx::B2DPoint aEndPoint( pHdl->getPosition() + aMove );
				    const SdrDragStat& rDragStat = rView.GetDragStat();

				    // start dragging
				    rView.BegDragObj( aStartPoint, pHdl, 0.0 );

				    if ( rView.IsDragObj() )
				    {
					    const bool bWasNoSnap = rDragStat.IsNoSnap();
					    const bool bWasSnapEnabled = rView.IsSnapEnabled();

					    // switch snapping off
					    if ( !bWasNoSnap )
						    ((SdrDragStat&)rDragStat).SetNoSnap( true );
					    if ( bWasSnapEnabled )
						    rView.SetSnapEnabled( false );

                        basegfx::B2DRange aNewRange;
                        bool bCheck = false;
						const SdrObjectVector aSelection(rView.getSelectedSdrObjectVectorFromSdrMarkView());

                        for (sal_uInt32 i(0); !bCheck && i < aSelection.size(); ++i )
                        {
                            bCheck = dynamic_cast< OUnoObject* >(aSelection[i]) != NULL
								|| dynamic_cast< OOle2Obj* >(aSelection[i]) != NULL;
                            if ( bCheck )
                                aNewRange.expand(aSelection[i]->getObjectRange(&rView));
                        }
                        
                        switch(pHdl->GetKind())
                        {
                            case HDL_LEFT:
                            case HDL_UPLFT:
                            case HDL_LWLFT:
                            case HDL_UPPER:
								aNewRange = basegfx::B2DRange(
									aNewRange.getMinimum() + aMove,
									aNewRange.getMaximum());
                                break;
                            case HDL_UPRGT:
                            case HDL_RIGHT:
                            case HDL_LWRGT:
                            case HDL_LOWER:
								aNewRange = basegfx::B2DRange(
									aNewRange.getMinimum(),
									aNewRange.getMinimum() + aMove);
                                break;
                            default:
                                break;
                        }
                        
						if ( !(bCheck && isOver(aNewRange,*rReportSection.getPage(),rView)) )
						{
                            rView.MovAction(aEndPoint);
						}

					    rView.EndDragObj();
    				
					    // restore snap
					    if ( !bWasNoSnap )
						    ((SdrDragStat&)rDragStat).SetNoSnap( bWasNoSnap );
					    if ( bWasSnapEnabled )
						    rView.SetSnapEnabled( bWasSnapEnabled );
				    }

				    // make moved handle visible
					const basegfx::B2DRange aRange(
						aEndPoint - basegfx::B2DPoint(DEFAUL_MOVE_SIZE, DEFAUL_MOVE_SIZE),
						aEndPoint + basegfx::B2DPoint(DEFAUL_MOVE_SIZE, DEFAUL_MOVE_SIZE));

				    rView.MakeVisibleAtView( aRange, rReportSection);
			    }
		    }

            rView.SetMarkHandles();
	    }
    }
}
// -----------------------------------------------------------------------------
void OViewsWindow::stopScrollTimer()
{
    ::std::for_each(m_aSections.begin(),m_aSections.end(),
		::std::compose1(::boost::mem_fn(&OReportSection::stopScrollTimer),TReportPairHelper()));
}
// -----------------------------------------------------------------------------
void OViewsWindow::fillCollapsedSections(::std::vector<sal_uInt16>& _rCollapsedPositions) const
{
    TSectionsMap::const_iterator aIter = m_aSections.begin();
	TSectionsMap::const_iterator aEnd = m_aSections.end();
    for (sal_uInt16 i = 0;aIter != aEnd ; ++aIter,++i)
	{
		if ( (*aIter)->getStartMarker().isCollapsed() )
            _rCollapsedPositions.push_back(i);
	}
}
// -----------------------------------------------------------------------------
void OViewsWindow::collapseSections(const uno::Sequence< beans::PropertyValue>& _aCollpasedSections)
{
    const beans::PropertyValue* pIter = _aCollpasedSections.getConstArray();
    const beans::PropertyValue* pEnd = pIter + _aCollpasedSections.getLength();
    for (; pIter != pEnd; ++pIter)
    {
        sal_uInt16 nPos = sal_uInt16(-1);
        if ( (pIter->Value >>= nPos) && nPos < m_aSections.size() )
        {
            m_aSections[nPos]->setCollapsed(sal_True);
        }
    }
}
// -----------------------------------------------------------------------------
void OViewsWindow::zoom(const Fraction& _aZoom)
{
    const MapMode& aMapMode = GetMapMode();
    
    Fraction aStartWidth(long(REPORT_STARTMARKER_WIDTH));
    if ( _aZoom < aMapMode.GetScaleX() )
        aStartWidth *= aMapMode.GetScaleX();
    else
        aStartWidth *= _aZoom;

    setZoomFactor(_aZoom,*this);

    TSectionsMap::iterator aIter = m_aSections.begin();
	TSectionsMap::iterator aEnd = m_aSections.end();
    for (;aIter != aEnd ; ++aIter)
	{
        (*aIter)->zoom(_aZoom);
    } // for (;aIter != aEnd ; ++aIter)

    Resize();
    
    Size aOut = GetOutputSizePixel();
    aOut.Width() = aStartWidth;
    aOut = PixelToLogic(aOut);
    
    Rectangle aRect(PixelToLogic(Point(0,0)),aOut);
    static sal_Int32 nIn = INVALIDATE_NOCHILDREN;
    Invalidate(aRect,nIn);
}
//----------------------------------------------------------------------------
void OViewsWindow::scrollChildren(const Point& _aThumbPos)
{
    const Point aPos(PixelToLogic(_aThumbPos));
    {
        MapMode aMapMode = GetMapMode();
        const Point aOld = aMapMode.GetOrigin();
        aMapMode.SetOrigin(m_pParent->GetMapMode().GetOrigin());

        const Point aPosY(m_pParent->PixelToLogic(_aThumbPos,aMapMode));
	    
	    aMapMode.SetOrigin( Point(aOld.X() , - aPosY.Y()));
	    SetMapMode( aMapMode );
	    //OWindowPositionCorrector aCorrector(this,0,-( aOld.Y() + aPosY.Y()));
	    Scroll(0, -( aOld.Y() + aPosY.Y()),SCROLL_CHILDREN);
    }

    TSectionsMap::iterator aIter = m_aSections.begin();
    TSectionsMap::iterator aEnd = m_aSections.end();
    for (;aIter != aEnd ; ++aIter)
    {
        (*aIter)->scrollChildren(aPos.X());
    } // for (;aIter != aEnd ; ++aIter)
}
// -----------------------------------------------------------------------------
void OViewsWindow::fillControlModelSelection(::std::vector< uno::Reference< uno::XInterface > >& _rSelection) const
{
    TSectionsMap::const_iterator aIter = m_aSections.begin();
    TSectionsMap::const_iterator aEnd = m_aSections.end();
    for(;aIter != aEnd; ++aIter)
    {
        (*aIter)->getReportSection().fillControlModelSelection(_rSelection);
    }
}
//==============================================================================
} // rptui
//==============================================================================