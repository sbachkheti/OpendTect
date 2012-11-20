#(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
# Description:  CMake script to build a release
# Author:       K. Tingdahl
# Date:		August 2012		
#RCS:           $Id: ODMakePackages.cmake,v 1.9 2012/09/11 12:25:44 cvsnageswara Exp $

#TODO Get version name from keyboard. 

SET( BASEPACKAGES basedatadefs dgbbasedatadefs)
SET( PACKAGELIST basedefs dgbbasedefs dgbccbdefs dgbdsdefs dgbhcdefs dgbnndefs dgbssisdefs dgbstratdefs dgbvmbdefs dgbwcpdefs odgmtdefs odgprdefs odmadagascardefs ) 

INCLUDE( CMakeModules/packagescripts/extlibs.cmake )
INCLUDE( CMakeModules/packagescripts/ODInstallReleaseStuff.cmake )
INCLUDE( CMakeModules/packagescripts/ODMakePackagesUtils.cmake )

foreach ( BASEPACKAGE ${BASEPACKAGES} )
    INCLUDE( ${PSD}/CMakeModules/packagescripts/${BASEPACKAGE}.cmake)
    init_destinationdir( ${PACK} )
    create_basepackages( ${PACK} )
endforeach()

foreach ( PACKAGE ${PACKAGELIST} )
    INCLUDE(CMakeModules/packagescripts/${PACKAGE}.cmake)
    MESSAGE( "Preparing package ${PACK}.zip ......" )
    IF( NOT DEFINED OpendTect_VERSION_MAJOR )
	MESSAGE( FATAL_ERROR "OpendTect_VERSION_MAJOR not defined" )
    ENDIF()

    IF( NOT DEFINED OD_PLFSUBDIR )
	MESSAGE( FATAL_ERROR "OD_PLFSUBDIR not defined" )
    ENDIF()

    IF( NOT EXISTS ${PSD}/inst )
	MESSAGE( FATAL_ERROR "${PSD}/inst is not existed. Do make install. " )
    ENDIF()

    IF( ${OD_PLFSUBDIR} STREQUAL "win32" OR ${OD_PLFSUBDIR} STREQUAL "win64" )
	IF( NOT EXISTS "${PSD}/bin/win/zip.exe" )
	    MESSAGE( FATAL_ERROR "${PSD}/bin/win/zip.exe is not existed.
		     Unable to create packages.Please do an update" )
	ENDIF()
    ENDIF()

    init_destinationdir( ${PACK} )
    create_package( ${PACK} )
endforeach()
MESSAGE( "\n Created packages are available under ${PSD}/packages" )
