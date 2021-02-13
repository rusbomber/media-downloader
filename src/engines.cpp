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

#include "engines.h"

#include "utility.h"

#include "engines/youtube-dl.h"
#include "engines/wget.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

#include <QDir>

const std::vector< engines::engine >& engines::getEngines()
{
	return m_backends ;
}

const engines::engine& engines::defaultEngine()
{
	return m_backends[ 0 ] ;
}

const engines::engine& engines::getEngine( const QString& url )
{
	for( size_t i = 1 ; i < m_backends.size() ; i++ ){

		const auto& m = m_backends[ i ] ;

		if( m.hasSupport( url ) ){

			return m ;
		}
	}

	return m_backends[ 0 ] ;
}

template< typename Engine >
static engines::engine _add_engine( const QString& enginePath,const Engine& engine )
{
	QJsonParseError error ;

	auto json = QJsonDocument::fromJson( engine.config( enginePath ),&error ) ;

	if( error.error != QJsonParseError::NoError ){

		return {} ;
	}

	auto object = json.object() ;

	auto usePrivateExecutable = object.value( "UsePrivateExecutable" ).toBool() ;

	auto commandName = object.value( "CommandName" ).toString() ;

	auto backendPath = object.value( "BackendPath" ).toString() ;

	auto downloadUrl = object.value( "DownloadUrl" ).toString() ;

	auto versionArgument = object.value( "VersionArgument" ).toString() ;

	auto optionsArgument = object.value( "OptionsArgument" ).toString() ;

	auto versionStringLine = object.value( "VersionStringLine" ).toInt() ;

	auto versionStringPosition = object.value( "VersionStringPosition" ).toInt() ;

	auto _toStringList = []( const QJsonValue& value ){

		QStringList m ;

		auto array = value.toArray() ;

		for( int i = 0 ; i < array.size() ;i ++ ){

			m.append( array.at( i ).toString() ) ;
		}

		return m ;
	} ;

	auto defaultDownLoadCmdOptions = _toStringList( object.value( "DefaultDownLoadCmdOptions" ) ) ;

	auto defaultListCmdOptions = _toStringList( object.value( "DefaultListCmdOptions" ) ) ;

	return engines::engine( engine.functions(),
				versionStringLine,
				versionStringPosition,
				usePrivateExecutable,
				commandName,
				backendPath,
				versionArgument,
				optionsArgument,
				downloadUrl,
				defaultDownLoadCmdOptions,
				defaultListCmdOptions ) ;
}

static QString _engine_path()
{
	auto m = QStandardPaths::standardLocations( QStandardPaths::AppDataLocation ) ;

	if( !m.isEmpty() ){

		return m.first() ;
	}else{
		//?????
		return QDir::homePath() + "/.config/media-downloader/" ;
	}
}

engines::engines()
{
	auto e = _engine_path() + "/engines" ;

	QDir().mkpath( e ) ;

	auto m = _add_engine( e,youtube_dl() ) ;

	if( m.valid() ){

		m_backends.emplace_back( std::move( m ) ) ;
	}else{
		//?????
	}
#if 0
	m = _add_engine( e,wget() ) ;

	if( m.valid() && !m.exePath().isEmpty() ){

		m_backends.emplace_back( std::move( m ) ) ;
	}else{
		//?????
	}
#endif
}

QString engines::engine::versionString( const QString& data ) const
{
	auto a = utility::split( data,'\n',true ) ;
	auto b = a[ m_line ] ;
	auto c = utility::split( b,' ',true ) ;
	return c[ m_position ] ;
}
