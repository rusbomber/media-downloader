/*
 *
 *  Copyright (c) 2021
 *  name : Francis Banyikwa
 *  email: mhogomchungu@gmail.com
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef UTILITY_H
#define UTILITY_H

#include <QStringList>
#include <QString>
#include <QProcess>
#include <QMenu>
#include <QPushButton>
#include <QTimer>
#include <QThread>

#include <type_traits>
#include <memory>

#include "translator.h"

#include "ui_mainwindow.h"

#include "engines.h"

class Context ;

class tabManager ;

namespace Ui
{
	class MainWindow ;
}

namespace utility
{
	namespace types
	{
		#if __cplusplus >= 201703L
			template<typename Function,typename ... Args>
			using result_of = std::invoke_result_t<Function,Args ...> ;
		#else
			template<typename Function,typename ... Args>
			using result_of = std::result_of_t<Function(Args ...)> ;
		#endif

		template<typename ReturnType,typename Function,typename ... Args>
		using imp = std::enable_if_t<std::is_same<result_of<Function,Args...>,ReturnType>::value,int> ;

		template<typename ReturnType,typename Function,typename ... Args>
		using has_same_return_type = std::enable_if_t<std::is_same<result_of<Function,Args...>,ReturnType>::value,int> ;

		template<typename Function,typename ... Args>
		using has_argument = imp<result_of<Function,Args...>,Function,Args...> ;

		template<typename Function>
		using has_no_argument = imp<result_of<Function>,Function> ;

		template<typename Function,typename ... Args>
		using has_void_return_type = has_same_return_type<void,Function,Args...> ;

		template<typename Function,typename ... Args>
		using has_bool_return_type = has_same_return_type<bool,Function,Args...> ;

		template<typename Function,typename ... Args>
		using has_non_void_return_type = std::enable_if_t<!std::is_void<result_of<Function,Args...>>::value,int> ;
	}

	template< typename T >
	typename std::add_const<T>::type& asConst( T& t )
	{
		return t ;
	}

	template< typename T >
	void asConst( const T&& ) = delete ;

	QStringList split( const QString& e,char token,bool skipEmptyParts ) ;
	QStringList split( const QString& e,const char * token ) ;
	QList< QByteArray > split( const QByteArray& e,char token = '\n' ) ;
	QList< QByteArray > split( const QByteArray& e,QChar token = '\n' ) ;
	QString failedToFindExecutableString( const QString& cmd ) ;

	class selectedAction
	{
	public:
		static const char * CLEARSCREEN ;
		static const char * CLEAROPTIONS ;
		static const char * OPENFOLDER ;

		selectedAction( QAction * ac ) : m_ac( ac )
		{
		}
		bool clearOptions() const
		{
			return m_ac->objectName() == utility::selectedAction::CLEAROPTIONS ;
		}
		bool clearScreen() const
		{
			return m_ac->objectName() == utility::selectedAction::CLEARSCREEN ;
		}
		bool openFolderPath() const
		{
			return m_ac->objectName() == utility::selectedAction::OPENFOLDER ;
		}
		QString text() const
		{
			return m_ac->text() ;
		}
		QString objectName() const
		{
			return m_ac->objectName() ;
		}
	private:
		QAction * m_ac ;
	};

	struct args
	{
		args( const QString& e )
		{
			if( !e.isEmpty() ){

				otherOptions = utility::split( e,' ',true ) ;

				if( !otherOptions.isEmpty() ){

					quality = otherOptions.takeFirst() ;
				}
			}
		}
		QString quality ;
		QStringList otherOptions ;
	} ;

	namespace details
	{
		QMenu * sMo( const Context&,
			     const QStringList& opts,
			     bool addClear,
			     QPushButton * w ) ;
	}

	template< typename Function >
	void setMenuOptions( const Context& ctx,
			     const QStringList& opts,
			     bool addClear,
			     QPushButton * w,
			     Function function )
	{
		auto menu = details::sMo( ctx,opts,addClear,w ) ;
		QObject::connect( menu,&QMenu::triggered,std::move( function ) ) ;
	}

	template< typename WhenCreated,
		  typename WhenStarted,
		  typename WhenDone,
		  typename WithData,
		  utility::types::has_non_void_return_type< WhenCreated,QProcess& > = 0 >
	void run( const QString& cmd,
		  const QStringList& args,
		  WhenCreated whenCreated,
		  WhenStarted whenStarted,
		  WhenDone whenDone,
		  WithData withData )
	{
		auto exe = new QProcess() ;

		using type = utility::types::result_of< WhenCreated,QProcess& > ;

		auto data = std::make_shared< type >( whenCreated( *exe ) ) ;

		QObject::connect( exe,&QProcess::readyReadStandardOutput,
				  [ exe,data,withData = std::move( withData ) ](){

			withData( QProcess::ProcessChannel::StandardOutput,
				  exe->readAllStandardOutput(),*data ) ;
		} ) ;

		QObject::connect( exe,&QProcess::readyReadStandardError,
				  [ exe,data,withData = std::move( withData ) ](){

			withData( QProcess::ProcessChannel::StandardError,
				  exe->readAllStandardError(),*data ) ;
		} ) ;

		using process = void( QProcess::* )( int,QProcess::ExitStatus ) ;

		auto s = static_cast< process >( &QProcess::finished ) ;

		QObject::connect( exe,s,[ data,exe,whenDone = std::move( whenDone ) ]
				  ( int e,QProcess::ExitStatus ss ){

			whenDone( e,ss,*data ) ;

			exe->deleteLater() ;
		} ) ;

		QObject::connect( exe,&QProcess::started,
				  [ exe,whenStarted = std::move( whenStarted ) ](){

			whenStarted( *exe ) ;
		} ) ;

		exe->start( cmd,args ) ;		
	}

	template< typename WhenCreated,
		  typename WhenStarted,
		  typename WhenDone,
		  typename WithData,
		  utility::types::has_void_return_type< WhenCreated,QProcess& > = 0 >
	void run( const QString& cmd,
		  const QStringList& args,
		  WhenCreated whenCreated,
		  WhenStarted whenStarted,
		  WhenDone whenDone,
		  WithData withData )
	{
		auto exe = new QProcess() ;

		whenCreated( *exe ) ;

		QObject::connect( exe,&QProcess::readyReadStandardOutput,
				  [ exe,withData = std::move( withData ) ](){

			withData( QProcess::ProcessChannel::StandardOutput,
				  exe->readAllStandardOutput() ) ;
		} ) ;

		QObject::connect( exe,&QProcess::readyReadStandardError,
				  [ exe,withData = std::move( withData ) ](){

			withData( QProcess::ProcessChannel::StandardError,
				  exe->readAllStandardError() ) ;
		} ) ;

		using type = void( QProcess::* )( int,QProcess::ExitStatus ) ;

		auto s = static_cast< type >( &QProcess::finished ) ;

		QObject::connect( exe,s,[ exe,whenDone = std::move( whenDone ) ]
				  ( int e,QProcess::ExitStatus ss ){

			whenDone( e,ss ) ;

			exe->deleteLater() ;
		} ) ;

		QObject::connect( exe,&QProcess::started,
				  [ exe,whenStarted = std::move( whenStarted ) ](){

			whenStarted( *exe ) ;
		} ) ;

		exe->start( cmd,args ) ;
	}

	template< typename WhenDone,typename WithData >
	void run( const QString& cmd,const QStringList& args,WhenDone w,WithData p )
	{
		utility::run( cmd,args,[]( QProcess& ){},std::move( w ),std::move( p ) ) ;
	}

	template< typename Object,
		  typename ObjectMemberFunction,
		  typename Function >
	struct Conn
	{
		Conn( Object obj,ObjectMemberFunction pointer,Function function ) :
			obj( obj ),
			pointer( pointer ),
			function( std::move( function ) )
		{
		}
		Object obj ;
		ObjectMemberFunction pointer ;
		Function function ;
	};

	template< typename Object,
		  typename ObjectMemberFunction,
		  typename Function >
	static auto make_conn( Object obj,ObjectMemberFunction memFunction,Function function )
	{
		return Conn< Object,ObjectMemberFunction,Function >( obj,memFunction,std::move( function ) ) ;
	}

	class ProcessExitState
	{
	public:
		ProcessExitState( bool c,int s,int d,QProcess::ExitStatus e ) :
			m_cancelled( c ),
			m_exitCode( s ),
			m_duration( d ),
			m_exitStatus( e )
		{
		}
		int exitCode() const
		{
			return m_exitCode ;
		}
		QProcess::ExitStatus exitStatus() const
		{
			return m_exitStatus ;
		}
		bool cancelled() const
		{
			return m_cancelled ;
		}
		bool success() const
		{
			return m_exitCode == 0 && m_exitStatus == QProcess::ExitStatus::NormalExit ;
		}
		int duration() const
		{
			return m_duration ;
		}
	private:
		bool m_cancelled = false ;
		int m_exitCode ;
		int m_duration ;
		QProcess::ExitStatus m_exitStatus ;
	};

	class ProcessOutputChannels
	{
	public:
		ProcessOutputChannels() :
			m_channelMode( QProcess::ProcessChannelMode::MergedChannels )
		{
		}
		ProcessOutputChannels( QProcess::ProcessChannel c ) :
			m_channelMode( QProcess::ProcessChannelMode::SeparateChannels ),
			m_channel( c )
		{
		}
		QProcess::ProcessChannelMode channelMode() const
		{
			return m_channelMode ;
		}
		QProcess::ProcessChannel channel() const
		{
			return m_channel ;
		}
	private:
		QProcess::ProcessChannelMode m_channelMode ;
		QProcess::ProcessChannel m_channel ;
	} ;

	template< typename Tlogger,
		  typename Options >
	class context
	{
	public:
		context( const engines::engine& engine,
			 Tlogger logger,
			 Options options,
			 ProcessOutputChannels channels ) :
			m_engine( engine ),
			m_cancelled( false ),
			m_logger( std::move( logger ) ),
			m_options( std::move( options ) ),
			m_channels( channels )
		{
			if( m_engine.replaceOutputWithProgressReport() ){

				QObject::connect( &m_timer,&QTimer::timeout,[ this ]{

					m_logger.add( [ this ]( Logger::Data& e,int id ){

						m_engine.processData( e,m_timeCounter.stringElapsedTime(),id ) ;
					} ) ;
				} ) ;

				m_timer.start( 1000 ) ;
			}
		}
		void setCancelConnection( QMetaObject::Connection conn )
		{
			m_conn = std::move( conn ) ;
		}
		void cancel()
		{
			m_cancelled = true ;
		}
		void postData( QByteArray data )
		{
			m_timer.stop() ;

			if( !m_cancelled ){

				if( m_options.listRequested() ){

					m_data += data ;
				}

				m_logger.add( [ this,data = std::move( data ) ]( Logger::Data& e,int id ){

					m_engine.processData( e,std::move( data ),id ) ;
				} ) ;
			}
		}
		bool debug()
		{
			return m_options.debug() ;
		}
		void done( int s,QProcess::ExitStatus e )
		{
			m_timer.stop() ;

			QObject::disconnect( m_conn ) ;

			if( m_options.listRequested() ){

				m_options.listRequested( utility::split( m_data,'\n' ) ) ;
			}

			auto m = m_timeCounter.elapsedTime() ;
			m_options.done( ProcessExitState( m_cancelled,s,m,std::move( e ) ) ) ;
		}
		const ProcessOutputChannels& outputChannels()
		{
			return m_channels ;
		}
	private:
		const engines::engine& m_engine ;
		bool m_cancelled ;
		QMetaObject::Connection m_conn ;
		Tlogger m_logger ;
		QByteArray m_data ;
		Options m_options ;
		ProcessOutputChannels m_channels ;
		QTimer m_timer ;
		engines::engine::functions::timer m_timeCounter ;
	} ;

	template< typename Connection,
		  typename Tlogger,
		  typename Options >
	void run( const engines::engine& engine,
		  const QStringList& args,
		  const QString& quality,
		  Options options,
		  Tlogger logger,
		  Connection conn,
		  ProcessOutputChannels channels = ProcessOutputChannels() )
	{
		engines::engine::exeArgs::cmd cmd( engine.exePath(),args ) ;

		const auto& exe = cmd.exe() ;

		if( !QFile::exists( exe ) ){

			logger.add( utility::failedToFindExecutableString( exe ) ) ;

			options.done( ProcessExitState( false,-1,-1,QProcess::ExitStatus::NormalExit ) ) ;

			return ;
		}

		options.disableAll() ;

		utility::run( exe,cmd.args(),[ &,logger = std::move( logger ),options = std::move( options ) ]( QProcess& exe )mutable{

			exe.setProcessEnvironment( options.processEnvironment() ) ;

			logger.add( "cmd: " + engine.commandString( cmd ) ) ;

			const auto& df = options.downloadFolder() ;

			if( !QFile::exists( df ) ){

				QDir().mkpath( df ) ;
			}

			exe.setWorkingDirectory( df ) ;

			exe.setProcessChannelMode( channels.channelMode() ) ;

			using ctx_t = utility::context< Tlogger,Options > ;

			auto ctx = std::make_shared< ctx_t >( engine,std::move( logger ),std::move( options ),channels ) ;
\
			auto fnt = [ &engine,&exe,ctx,function = std::move( conn.function ) ](){

				ctx->cancel() ;

				function( engine,exe ) ;
			} ;

			ctx->setCancelConnection( QObject::connect( conn.obj,conn.pointer,std::move( fnt ) ) ) ;

			return ctx ;

		},[ &engine,quality ]( QProcess& exe ){

			engine.sendCredentials( quality,exe ) ;

		},[]( int s,QProcess::ExitStatus e,std::shared_ptr< utility::context< Tlogger,Options > >& ctx ){

			ctx->done( s,std::move( e ) ) ;

		},[]( QProcess::ProcessChannel channel,QByteArray data,std::shared_ptr< utility::context< Tlogger,Options > >& ctx ){

			if( ctx->debug() ){

				qDebug() << data ;
				qDebug() << "------------------------" ;
			}

			const auto& channels = ctx->outputChannels() ;

			if( channels.channelMode() == QProcess::ProcessChannelMode::MergedChannels ){

				ctx->postData( std::move( data ) ) ;

			}else if( channels.channelMode() == QProcess::ProcessChannelMode::SeparateChannels ){

				auto c = channels.channel() ;

				if( c == QProcess::ProcessChannel::StandardOutput && channel == c ){

					ctx->postData( std::move( data ) ) ;

				}else if( c == QProcess::ProcessChannel::StandardError && channel == c ){

					ctx->postData( std::move( data ) ) ;
				}
			}
		} ) ;
	}

	/*
	 * Function must take an int and must return bool
	 */
	template< typename Function,utility::types::has_bool_return_type<Function,int > = 0 >
	void Timer( int interval,Function&& function )
	{
		class Timer{
		public:
			Timer( int interval,Function&& function ) :
				m_function( std::forward< Function >( function ) )
			{
				auto timer = new QTimer() ;

				QObject::connect( timer,&QTimer::timeout,[ timer,this ](){

					m_counter++ ;

					if( m_function( m_counter ) ){

						timer->stop() ;

						timer->deleteLater() ;

						delete this ;
					}
				} ) ;

				timer->start( interval ) ;
			}
		private:
			int m_counter = 0 ;
			Function m_function ;
		} ;

		new Timer( interval,std::forward< Function >( function ) ) ;
	}

	/*
	 * Function must takes no argument and will be called once when the interval pass
	 */
	template< typename Function,utility::types::has_no_argument< Function > = 0 >
	void Timer( int interval,Function&& function )
	{
		utility::Timer( interval,[ function = std::forward< Function >( function ) ]( int s ){

			Q_UNUSED( s )

			function() ;

			return true ;
		} ) ;
	}

	template< typename List,
		  std::enable_if_t< std::is_lvalue_reference< List >::value,int > = 0 >
	class reverseIterator
	{
	public:
		typedef typename std::remove_reference_t< std::remove_cv_t< List > > ::value_type value_type ;
		typedef typename std::remove_reference_t< std::remove_cv_t< List > > ::size_type size_type ;

	        reverseIterator( List s ) :
		        m_list( s ),
			m_index( m_list.size() - 1 )
		{
		}
		bool hasNext() const
		{
			return m_index > -1 ;
		}
		void reset()
		{
			m_index = m_list.size() - 1 ;
		}
		auto& next()
		{
			auto s = static_cast< typename reverseIterator< List >::size_type >( m_index-- ) ;

			return m_list[ s ] ;
		}
		template< typename Function,
			  utility::types::has_bool_return_type< Function,typename reverseIterator< List >::value_type > = 0 >
		void forEach( Function function )
		{
			while( this->hasNext() ){

				if( function( this->next() ) ){

					break ;
				}
			}
		}
		template< typename Function,
			  utility::types::has_void_return_type< Function,typename reverseIterator< List >::value_type > = 0 >
		void forEach( Function function )
		{
			while( this->hasNext() ){

				function( this->next() ) ;
			}
		}
	private:
		List m_list ;
		int m_index ;
	} ;

	template< typename List >
	auto make_reverseIterator( List&& l )
	{
		return reverseIterator< decltype( l ) >( std::forward< List >( l ) ) ;
	}

	template< typename S >
	class storage
	{
	public:
		template< typename ... T >
		void create( T&& ... t )
		{
			if( m_pointer ){

				m_pointer->~S() ;
			}

			m_pointer = new ( &m_storage ) S( std::forward< T >( t ) ... ) ;
		}
		S& get()
		{
			return *m_pointer ;
		}
		S * operator->()
		{
			return m_pointer ;
		}
		~storage()
		{
			if( m_pointer ){

				m_pointer->~S() ;
			}
		}
		bool created()
		{
			return m_pointer ;
		}
	private:
		S * m_pointer = nullptr ;
		#if __cplusplus >= 201703L
			alignas( S ) std::byte m_storage[ sizeof( S ) ] ;
		#else
			typename std::aligned_storage< sizeof( S ),alignof( S ) >::type m_storage ;
		#endif
	};

	template< typename BackGroundTask,
		  typename UiThreadResult >
	class Thread : public QThread
	{
	public:
		Thread( BackGroundTask bgt,UiThreadResult fgt ) :
			m_bgt( std::move( bgt ) ),
			m_fgt( std::move( fgt ) )
		{
			connect( this,&QThread::finished,this,&Thread::then,Qt::QueuedConnection ) ;

			this->start() ;
		}
		void run() override
		{
			m_storage.create( m_bgt() ) ;
		}
		void then()
		{
			m_fgt( std::move( m_storage.get() ) ) ;

			this->deleteLater() ;
		}
	private:
		BackGroundTask m_bgt ;
		UiThreadResult m_fgt ;

		utility::storage< utility::types::result_of< BackGroundTask > > m_storage ;
	};

	template< typename BackGroundTask,
		  typename UiThreadResult,
		  utility::types::has_non_void_return_type< BackGroundTask > = 0 >
	void runInBgThread( BackGroundTask bgt,UiThreadResult fgt )
	{
		new Thread< BackGroundTask,UiThreadResult >( std::move( bgt ),std::move( fgt ) ) ;
	}

	template< typename BackGroundTask,
		  typename UiThreadResult,
		  utility::types::has_void_return_type< BackGroundTask > = 0 >
	void runInBgThread( BackGroundTask bgt,UiThreadResult fgt )
	{
		return utility::runInBgThread( [ bgt = std::move( bgt ) ](){

			bgt() ;

			return 0 ;

		},[ fgt = std::move( fgt ) ]( int ){

			fgt() ;
		} ) ;
	}

	template< typename BackGroundTask >
	void runInBgThread( BackGroundTask bgt )
	{
		return utility::runInBgThread( [ bgt = std::move( bgt ) ](){

			bgt() ;

			return 0 ;

		},[]( int ){} ) ;
	}

	template< typename FinishedState >
	void updateFinishedState( const engines::engine& engine,
				  settings& s,
				  QTableWidget& table,
				  const FinishedState& f )
	{
		const auto& index = f.index() ;
		const auto& es = f.exitState() ;

		f.setState( *table.item( index,2 ) ) ;

		const auto backUpUrl = table.item( index,1 )->text() ;

		auto item = table.item( index,0 ) ;

		auto a = item->text() ;

		item->setText( engine.updateTextOnCompleteDownlod( a,backUpUrl,es ) ) ;

		if( !es.cancelled() ){

			if( es.success() ){

				engine.runCommandOnDownloadedFile( a,backUpUrl ) ;
			}

			if( f.allFinished() ){

				auto a = s.commandWhenAllFinished() ;

				if( !a.isEmpty() ){

					auto args = utility::split( a,' ',true ) ;

					auto exe = args.takeAt( 0 ) ;

					QProcess::startDetached( exe,args ) ;
				}
			}
		}
	}

	int concurrentID() ;

	void wait( int time ) ;
	void waitForOneSecond() ;
	void openDownloadFolderPath( const QString& ) ;
	QString homePath() ;
	QString python3Path() ;
	QString clipboardText() ;
	int terminateProcess( unsigned long pid ) ;
	void terminateProcess( const engines::engine&,QProcess& ) ;
	bool platformIsWindows() ;
	bool platformIs32BitWindows() ;
	bool platformIsLinux() ;
	bool platformIsOSX() ;
	bool platformIsNOTWindows() ;

	QString downloadFolder( const Context& ctx ) ;
	const QProcessEnvironment& processEnvironment( const Context& ctx ) ;

	QStringList updateOptions( const engines::engine& engine,
				   settings&,
				   const utility::args& args,
				   const QStringList& urls ) ;

	bool hasDigitsOnly( const QString& e ) ;

	template< typename Object,
		  typename ObjectMemberFunction >
	static auto make_term_conn( Object obj,ObjectMemberFunction memFunction )
	{
		auto s = static_cast< void( * )( const engines::engine&,QProcess& ) >( utility::terminateProcess ) ;

		return utility::make_conn( obj,memFunction,s ) ;
	}

	struct opts
	{
		const Context& ctx ;
		const engines::engine& engine ;
		QTableWidget& table ;
		bool debug ;
		bool listRequested ;
		QString downloadFolder() const ;
		const QProcessEnvironment& processEnvironment() const ;
	} ;

	template< typename Opts,typename Functions >
	class options
	{
	public:
		options( Opts opts,Functions functions ) :
			m_opts( std::move( opts ) ),
			m_functions( std::move( functions ) )
		{
		}
		void done( utility::ProcessExitState e )
		{
			m_functions.done( std::move( e ),m_opts ) ;
		}
		void listRequested( const QList< QByteArray >& e )
		{
			m_functions.list( e ) ;
		}
		bool listRequested()
		{
			return m_opts.listRequested ;
		}
		bool debug()
		{
			return m_opts.debug ;
		}
		void disableAll()
		{
			m_functions.disableAll( m_opts ) ;
		}
		QString downloadFolder() const
		{
			return utility::downloadFolder( m_opts.ctx ) ;
		}
		const QProcessEnvironment& processEnvironment() const
		{
			return utility::processEnvironment( m_opts.ctx ) ;
		}
	private:
		Opts m_opts ;
		Functions m_functions ;
	} ;

	template< typename List,typename DisableAll,typename Done >
	struct Functions
	{
		List list ;
		DisableAll disableAll ;
		Done done ;
	} ;

	template< typename List,typename DisableAll,typename Done >
	Functions< List,DisableAll,Done > OptionsFunctions( List list,DisableAll disableAll,Done done )
	{
		return { std::move( list ),std::move( disableAll ),std::move( done ) } ;
	}

	template< typename DisableAll,typename Done >
	auto OptionsFunctions( DisableAll disableAll,Done done )
	{
		auto aa = []( const QList< QByteArray >& ){} ;

		using type = Functions< decltype( aa ),DisableAll,Done > ;

		return type{ std::move( aa ),std::move( disableAll ),std::move( done ) } ;
	}
}

#endif
