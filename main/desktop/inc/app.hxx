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



#ifndef _DESKTOP_APP_HXX_
#define _DESKTOP_APP_HXX_

// stl includes first
#include <map>
#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#include <vcl/svapp.hxx>
#ifndef _VCL_TIMER_HXX_
#include <vcl/timer.hxx>
#endif
#include <tools/resmgr.hxx>
#include <unotools/bootstrap.hxx>
#include <com/sun/star/lang/XInitialization.hpp>
#include <com/sun/star/task/XStatusIndicator.hpp>
#include <com/sun/star/uno/Reference.h>
#include <osl/mutex.hxx>

using namespace com::sun::star::task;
using namespace com::sun::star::uno;
using namespace com::sun::star::lang;
using namespace rtl;

#define DESKTOP_SAVETASKS_MOD 0x1
#define DESKTOP_SAVETASKS_UNMOD 0x2
#define DESKTOP_SAVETASKS_ALL 0x3

namespace desktop
{

/*--------------------------------------------------------------------
	Description:	Application-class
 --------------------------------------------------------------------*/
class CommandLineArgs;
class Lockfile;
class AcceptorMap : public std::map< OUString, Reference<XInitialization> > {};
struct ConvertData;
class Desktop : public Application
{
    friend class UserInstall;

    void doShutdown();

	public:
		enum BootstrapError
		{
			BE_OK,
			BE_UNO_SERVICEMANAGER,
			BE_UNO_SERVICE_CONFIG_MISSING,
			BE_PATHINFO_MISSING,
            BE_USERINSTALL_FAILED,
            BE_LANGUAGE_MISSING,
            BE_USERINSTALL_NOTENOUGHDISKSPACE,
            BE_USERINSTALL_NOWRITEACCESS,
            BE_MUTLISESSION_NOT_SUPPROTED,
            BE_OFFICECONFIG_BROKEN
		};
        enum BootstrapStatus
        {
            BS_OK,
            BS_TERMINATE
        };

								Desktop();
								~Desktop();
		virtual void			Main( );
		virtual void			Init();
		virtual void			DeInit();
		virtual sal_Bool			QueryExit();
		virtual sal_uInt16			Exception(sal_uInt16 nError);
		virtual void			SystemSettingsChanging( AllSettings& rSettings, Window* pFrame );
		virtual void			AppEvent( const ApplicationEvent& rAppEvent );
		
		DECL_LINK(          OpenClients_Impl, void* );

		static void				OpenClients();
		static void				OpenDefault();

		DECL_LINK( EnableAcceptors_Impl, void*);

		static void				HandleAppEvent( const ApplicationEvent& rAppEvent );
		static ResMgr*			GetDesktopResManager();
		static CommandLineArgs* GetCommandLineArgs();

		void					HandleBootstrapErrors( BootstrapError );
		void					SetBootstrapError( BootstrapError nError )
		{
			if ( m_aBootstrapError == BE_OK )
				m_aBootstrapError = nError;
		}
        BootstrapError          GetBootstrapError() const
        {
            return m_aBootstrapError;
        }

        void					SetBootstrapStatus( BootstrapStatus nStatus )
		{
            m_aBootstrapStatus = nStatus;
		}
        BootstrapStatus          GetBootstrapStatus() const
        {
            return m_aBootstrapStatus;
        }        
        
		static sal_Bool         CheckOEM();
        static sal_Bool         isCrashReporterEnabled();

        // first-start (ever) & license relate methods
        static rtl::OUString    GetLicensePath();
        static sal_Bool         LicenseNeedsAcceptance();
        static sal_Bool         IsFirstStartWizardNeeded();
        static sal_Bool         CheckExtensionDependencies();
        static void             EnableQuickstart();
        static void             FinishFirstStart();

        static void             DoRestartActionsIfNecessary( sal_Bool bQuickStart );
        static void             SetRestartState();

        void                    SynchronizeExtensionRepositories();
        void                    SetSplashScreenText( const ::rtl::OUString& rText );
        void                    SetSplashScreenProgress( sal_Int32 );

	private:
		// Bootstrap methods
		::com::sun::star::uno::Reference< ::com::sun::star::lang::XMultiServiceFactory > CreateApplicationServiceManager();

		void					RegisterServices( ::com::sun::star::uno::Reference< ::com::sun::star::lang::XMultiServiceFactory >& xSMgr );
		void					DeregisterServices();

		void					DestroyApplicationServiceManager( ::com::sun::star::uno::Reference< ::com::sun::star::lang::XMultiServiceFactory >& xSMgr );

		void					CreateTemporaryDirectory();
		void					RemoveTemporaryDirectory();

		sal_Bool				InitializeInstallation( const rtl::OUString& rAppFilename );
		sal_Bool				InitializeConfiguration();
        void                    FlushConfiguration();
		sal_Bool				InitializeQuickstartMode( com::sun::star::uno::Reference< com::sun::star::lang::XMultiServiceFactory >& rSMgr );

		void					HandleBootstrapPathErrors( ::utl::Bootstrap::Status, const ::rtl::OUString& aMsg );
		void					StartSetup( const ::rtl::OUString& aParameters );

		// Get a resource message string securely e.g. if resource cannot be retrieved return aFaultBackMsg
		::rtl::OUString			GetMsgString( sal_uInt16 nId, const ::rtl::OUString& aFaultBackMsg );

		// Create a error message depending on bootstrap failure code and an optional file url
		::rtl::OUString			CreateErrorMsgString( utl::Bootstrap::FailureCode nFailureCode,
													  const ::rtl::OUString& aFileURL );

		static void             PreloadModuleData( CommandLineArgs* );
        static void             PreloadConfigurationData();
        
        Reference<XStatusIndicator> m_rSplashScreen;
        void                    OpenSplashScreen();
        void                    CloseSplashScreen();

    	void					EnableOleAutomation();
								DECL_LINK( ImplInitFilterHdl, ConvertData* );
		DECL_LINK(			AsyncInitFirstRun, void* );
		/** checks if the office is run the first time
			<p>If so, <method>DoFirstRunInitializations</method> is called (asynchronously and delayed) and the
			respective flag in the configuration is reset.</p>
		*/
		void					CheckFirstRun( );

		/// does initializations which are necessary for the first run of the office
		void					DoFirstRunInitializations();

		static sal_Bool			SaveTasks();
        static sal_Bool         _bTasksSaved;

        static void             retrieveCrashReporterState();
        static sal_Bool         isUIOnSessionShutdownAllowed();

        // on-demand acceptors
		static void							createAcceptor(const OUString& aDescription);
		static void							enableAcceptors();
		static void							destroyAcceptor(const OUString& aDescription);

		sal_Bool						m_bMinimized;
		sal_Bool						m_bInvisible;
		bool                            m_bServicesRegistered;
		sal_uInt16							m_nAppEvents;
		BootstrapError					m_aBootstrapError;
        BootstrapStatus                 m_aBootstrapStatus;

		Lockfile *m_pLockfile;
        Timer    m_firstRunTimer;

		static ResMgr*					pResMgr;
        static sal_Bool                 bSuppressOpenDefault;
};

}

#endif // _DESKTOP_APP_HXX_
