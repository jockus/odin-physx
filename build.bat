@echo off

rem goto skip_physx

rem Build physx
if not exist physx_lib (
	mkdir physx_lib
)
pushd physx_lib
	set "root=%~dp0\Physx"
	rem Backslash to forward slash - the PhysX cmake files doesn't handle backslash
	set "root=%root:\=/%"

	cmake -G "NMake Makefiles"^
	 %root%/physx/compiler/public/^
	 -DTARGET_BUILD_PLATFORM=windows^
	 -DPX_OUTPUT_ARCH=x86^
	 --no-warn-unused-cli^
	 -DCMAKE_PREFIX_PATH="%root%/externals/CMakeModules;%root%/externals/targa"^
	 -DPHYSX_ROOT_DIR="%root%/physx"^
	 -DPX_OUTPUT_LIB_DIR="lib"^
	 -DPX_OUTPUT_BIN_DIR="%root%/physx"^
	 -DPX_BUILDSNIPPETS=FALSE^
	 -DPX_BUILDPUBLICSAMPLES=FALSE^
	 -DPX_GENERATE_STATIC_LIBRARIES=TRUE^
	 -DNV_USE_STATIC_WINCRT=TRUE^
	 -DNV_USE_DEBUG_WINCRT=FALSE^
	 -DPX_FLOAT_POINT_PRECISE_MATH=FALSE^
	 -DPX_PHYSX_STATIC_LIB=1^
	 -DCMAKE_BUILD_TYPE=release^
	 && nmake
	
	rem copy libs to physx_lib folder for easier linking
	for /R "%~dp0" %%F in ("*.lib") do copy %%~F .
popd


:skip_physx

rem Compile c wrapper lib
if not exist lib (
	mkdir lib
)

set libs=^
 ..\physx_lib\PhysX_static_64.lib^
 ..\physx_lib\PhysXCommon_static_64.lib^
 ..\physx_lib\PhysXExtensions_static_64.lib^
 ..\physx_lib\PhysXFoundation_static_64.lib^
 ..\physx_lib\PhysXPvdSDK_static_64.lib


pushd lib
cl.exe /c /DEBUG /EHsc -MP -DNDEBUG -DPX_PHYSX_STATIC_LIB -I..\Physx\physx\include -I..\Physx\pxshared\include ..\physx_lib.cpp
lib.exe physx_lib.obj %libs%
popd lib
