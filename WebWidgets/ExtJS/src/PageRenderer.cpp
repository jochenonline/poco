//
// PageRenderer.cpp
//
// $Id: //poco/Main/WebWidgets/ExtJS/src/PageRenderer.cpp#8 $
//
// Library: ExtJS
// Package: Core
// Module:  PageRenderer
//
// Copyright (c) 2007, Applied Informatics Software Engineering GmbH.
// and Contributors.
//
// Permission is hereby granted, free of charge, to any person or organization
// obtaining a copy of the software and accompanying documentation covered by
// this license (the "Software") to use, reproduce, display, distribute,
// execute, and transmit the Software, and to prepare derivative works of the
// Software, and to permit third-parties to whom the Software is furnished to
// do so, all subject to the following:
// 
// The copyright notices in the Software and this entire statement, including
// the above license grant, this restriction and the following disclaimer,
// must be included in all copies of the Software, in whole or in part, and
// all derivative works of the Software, unless such copies or derivative
// works are solely in the form of machine-executable object code generated by
// a source language processor.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
// SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
// FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//


#include "Poco/WebWidgets/ExtJS/PageRenderer.h"
#include "Poco/WebWidgets/ExtJS/Utility.h"
#include "Poco/WebWidgets/ExtJS/PanelRenderer.h"
#include "Poco/WebWidgets/Page.h"
#include "Poco/WebWidgets/Panel.h"
#include "Poco/WebWidgets/RenderContext.h"
#include "Poco/WebWidgets/LookAndFeel.h"
#include "Poco/WebWidgets/WebApplication.h"
#include "Poco/WebWidgets/RequestHandler.h"
#include "Poco/NumberFormatter.h"


namespace Poco {
namespace WebWidgets {
namespace ExtJS {


const std::string PageRenderer::EV_BEFORERENDER("beforerender");
const std::string PageRenderer::EV_AFTERRENDER("show"); //don't use afterrender which fires before all the children are rendered!
const std::string PageRenderer::VAR_LOCALTMP("tmpLocal");


PageRenderer::PageRenderer()
{
}


PageRenderer::~PageRenderer()
{
}


void PageRenderer::renderHead(const Renderable* pRenderable, const RenderContext& context, std::ostream& ostr)
{
	static const std::string STRO_HTML("<html>");
	static const std::string STRO_HEAD ("<head>");
	static const std::string STRO_TITLE ("<title>");
	static const std::string STRC_HEAD ("</head>");
	static const std::string STRC_TITLE ("</title>");
	poco_assert_dbg (pRenderable != 0);
	poco_assert_dbg (pRenderable->type() == typeid(Poco::WebWidgets::Page));
	Page* pPage = const_cast<Page*>(static_cast<const Poco::WebWidgets::Page*>(pRenderable));
	pPage->pageRequested.notify(this, pPage);
	
	poco_assert_dbg (context.application().getCurrentPage().get() == pPage);
	
	const LookAndFeel& laf = context.lookAndFeel();
	ResourceManager::Ptr pRM = context.application().getResourceManager();

	ostr << STRO_HTML << STRO_HEAD << STRO_TITLE;
	ostr << pPage->getName();
	ostr << STRC_TITLE;
	//include javascript files: 
	const ResourceManager::Includes& jsIncl = pRM->jsIncludes();
	ResourceManager::Includes::const_iterator itJS = jsIncl.begin();
	for (; itJS != jsIncl.end(); ++itJS)
	{
		ostr << "<script type=\"text/javascript\" src=\"" << (*itJS) << "\"></script>";
	}
	const ResourceManager::Includes& myJSIncl = pPage->resourceManager().jsIncludes();
	ResourceManager::Includes::const_iterator itMyJS = myJSIncl.begin();
	for (; itMyJS != myJSIncl.end(); ++itMyJS)
	{
		ostr << "<script type=\"text/javascript\" src=\"" << (*itMyJS) << "\"></script>";
	}
	
	// write css includes
	const ResourceManager::Includes& cssIncl = pRM->cssIncludes();
	ResourceManager::Includes::const_iterator itCSS = cssIncl.begin();
	for (; itCSS != cssIncl.end(); ++itCSS)
	{
		ostr << "<link rel=\"stylesheet\" type=\"text/css\" href=\"" << (*itCSS) << "\">";
	}
	const ResourceManager::Includes& myCSSIncl = pPage->resourceManager().cssIncludes();
	ResourceManager::Includes::const_iterator itMyCSS = myCSSIncl.begin();
	for (; itMyCSS != myCSSIncl.end(); ++itMyCSS)
	{
		ostr << "<link rel=\"stylesheet\" type=\"text/css\" href=\"" << (*itMyCSS) << "\">";
	}
	// extra css  for label
	ostr << "<style type=\"text/css\">.lbl {font:normal 12px tahoma, verdana, helvetica}</style>";
	ostr << "<style type=\"text/css\">.x-form-cb-label {font:normal 12px tahoma, verdana, helvetica}</style>";
	
	if (!pPage->empty())
	{
		//start inline javascript block
		ostr << "<script type=\"text/javascript\">";
		ostr << "var global={};"; //global var to store values!
		ostr << "var winGrp = new Ext.WindowGroup();";
		ostr <<	"winGrp.zseed = 5000;";
		const std::vector<std::string>& fcts = pPage->dynamicFunctions();
		std::vector<std::string>::const_iterator itF = fcts.begin();
		for (; itF != fcts.end(); ++itF)
		{
			ostr << *itF << std::endl;
		}
		
		const std::set<DynamicCodeLoader::Ptr>& dcls = pPage->dynamicCodeLoaders();
		if (!dcls.empty())
		{
			ostr <<	"function loadScriptDynamically(sId, source){" << std::endl;
			ostr <<		"if (!source) return;" << std::endl;
			ostr <<		"var oHead = document.getElementsByTagName('HEAD').item(0);" << std::endl;
			ostr <<		"var oScript = document.createElement('script');" << std::endl;
			ostr <<		"oScript.language = 'javascript';" << std::endl;
			ostr <<		"oScript.type = 'text/javascript';" << std::endl;
			ostr <<		"oScript.id = sId;" << std::endl;
			ostr <<		"oScript.defer = true;" << std::endl;
			ostr <<		"oScript.text = source.responseText;" << std::endl;
			ostr <<		"oHead.appendChild(oScript);" << std::endl;
			ostr <<	"}" << std::endl;
		}
		std::set<DynamicCodeLoader::Ptr>::const_iterator itDC = dcls.begin();
		for (; itDC != dcls.end(); ++itDC)
		{
			(*itDC)->renderHead(context, ostr);
		}
		
		
		
		ostr << "Ext.onReady(function() {";
		ostr << "var " << VAR_LOCALTMP << ";"; // tmp variable needed for table renderer
		ostr << "Ext.QuickTips.init();";
		ostr <<	"Ext.Ajax.timeout=60000;"; // increase the timeout to 60 secs
		ostr << "Ext.Ajax.on({'requestexception':function(conn, resp, obj){";
		ostr <<				"Ext.Msg.show({";
		ostr <<					"title:'Server Error',";
		ostr <<					"msg:resp.statusText,";
		ostr <<					"buttons:Ext.Msg.OK,";
		ostr <<					"icon:Ext.MessageBox.ERROR";
		ostr <<				"});";
		ostr <<			"}});";
	
		// always nest a panel around, so we can get rid of dynamic casts to check for parent type
		ostr << "new Ext.Panel({renderTo:'p" << pPage->id() << "',border:false,bodyBorder:false,autoScroll:true,id:'" << pPage->id() << "'";
		if (pPage->beforeRender.hasJavaScriptCode() || pPage->afterRender.hasJavaScriptCode())
		{
			ostr << ",listeners:{";
			bool written = Utility::writeJSEvent(ostr, EV_BEFORERENDER, pPage->beforeRender, &PageRenderer::createBeforeRenderCallback, pPage);
			if (pPage->afterRender.hasJavaScriptCode())
			{
				if (written)
					ostr << ",";
				Utility::writeJSEvent(ostr, EV_AFTERRENDER, pPage->afterRender, &PageRenderer::createAfterRenderCallback, pPage);
			}
			ostr << "}";
		}

		if (pPage->getHeight() > 0)
			ostr << ",height:" << pPage->getHeight();
		else
			ostr << ",height:Ext.lib.Dom.getViewHeight()";
		if (pPage->getWidth() > 0)
			ostr << ",width:" << pPage->getWidth();
		else
			ostr << ",width:Ext.lib.Dom.getViewWidth()-20";
		
		ostr << ",items:[";
		// write an empty hull for the dynamiccodeloadres: solves z-seed problem
		std::set<DynamicCodeLoader::Ptr>::const_iterator itDCL = dcls.begin();
		AutoPtr<PanelRenderer> pPanelRenderer(new PanelRenderer());
		bool writeComma = false;
		for (; itDCL != dcls.end(); ++itDCL)
		{
			View::Ptr pView = (*itDCL)->view();
			AutoPtr<Panel> pPanel = pView.cast<Panel>();
			if (pPanel)
			{
				if (writeComma)
				{
					ostr << ",";
				}
				pPanelRenderer->renderHeadWithoutChildren(pPanel, context, ostr);
				writeComma = true;
			}
		}
		//process all children
		ContainerView::ConstIterator it = pPage->begin();
		ContainerView::ConstIterator itEnd = pPage->end();
		if (!dcls.empty() && it != itEnd)
			ostr << ",";
		
		for (; it != itEnd; ++it)
		{
			if (it != pPage->begin())
				ostr << ",";

			(*it)->renderHead(context, ostr);
		}
		//close the panel
		ostr << "]});";
		if (!pPage->getPostRenderCode().empty())
			ostr << pPage->getPostRenderCode();
		//close onReady
		ostr << "});";
		//close inline JS block
		ostr << "</script>";
	}

	ostr << STRC_HEAD;
	WebApplication::instance().registerAjaxProcessor(Poco::NumberFormatter::format(pPage->id()), pPage);
	pPage->afterPageRequested.notify(this, pPage);
}


void PageRenderer::renderBody(const Renderable* pRenderable, const RenderContext& context, std::ostream& ostr)
{
	poco_assert_dbg (pRenderable != 0);
	poco_assert_dbg (pRenderable->type() == typeid(Poco::WebWidgets::Page));
	const Page* pPage = static_cast<const Poco::WebWidgets::Page*>(pRenderable);
	const LookAndFeel& laf = context.lookAndFeel();

	ostr << "<body>";
	if (!pPage->empty())
	{
		//process all children: ExtJS is a JavaScript library, we NEVER write to the body
		// except for Panel!
		ostr << "<div id=\"p" << pPage->id() << "\" />";
		// also a tmp id for temporary storage!
		ostr << "<div id=\"" << Utility::getTmpID() << "\" />";
		/*
		ContainerView::ConstIterator it = pPage->begin();
		ContainerView::ConstIterator itEnd = pPage->end();
		for (; it != itEnd; ++it)
		{
			if (it != pPage->begin())
				ostr << ",";

			(*it)->renderBody(context, ostr);
		}
		*/
	}
	ostr << "</body>";
	ostr << "</html>";
}


Poco::WebWidgets::JSDelegate PageRenderer::createBeforeRenderCallback(const Page* pPage)
{
	// JS signature: beforerender : ( Ext.Component this )
	static const std::string signature("function(p)");
	std::map<std::string, std::string> addParams;
	addParams.insert(std::make_pair(RequestHandler::KEY_EVID, Page::EV_BEFORERENDER));
	return Utility::createServerCallback(signature, addParams, pPage->id(), pPage->beforeRender.getOnSuccess(), pPage->beforeRender.getOnFailure());
}


Poco::WebWidgets::JSDelegate PageRenderer::createAfterRenderCallback(const Page* pPage)
{
	// JS signature: show : ( Ext.Component this )
	static const std::string signature("function(p)");
	std::map<std::string, std::string> addParams;
	addParams.insert(std::make_pair(RequestHandler::KEY_EVID, Page::EV_AFTERRENDER));
	return Utility::createServerCallback(signature, addParams, pPage->id(), pPage->afterRender.getOnSuccess(), pPage->afterRender.getOnFailure());
}


} } } // namespace Poco::WebWidgets::ExtJS
