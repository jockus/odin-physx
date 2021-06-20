@echo off
setlocal

rem Build physx
if not exist physx_lib (
	mkdir physx_lib
)
pushd physx_lib
	set "root=%~dp0\Physx"
	rem Backslash to forward slash - the PhysX cmake files doesn't handle backslash
	set "root=%root:\=/%"

	cmake -G "NMake Makefiles"^
	 %root%/physx/source/compiler/cmake/^
	 -DTARGET_BUILD_PLATFORM=windows^
	 -DPX_OUTPUT_ARCH=x86^
	 --no-warn-unused-cli^
	 -DCMAKE_PREFIX_PATH="%root%/externals/CMakeModules;%root%/externals/targa"^
	 -DPHYSX_ROOT_DIR="%root%/physx"^
	 -DPX_OUTPUT_LIB_DIR="%root%/lib"^
	 -DPX_OUTPUT_BIN_DIR="%root%/physx"^
	 -DPX_BUILDSNIPPETS=FALSE^
	 -DPX_BUILDPUBLICSAMPLES=FALSE^
	 -DNV_USE_STATIC_WINCRT=TRUE^
	 -DNV_USE_DEBUG_WINCRT=FALSE^
	 -DPX_FLOAT_POINT_PRECISE_MATH=FALSE^
	 -DCMAKE_BUILD_TYPE=profile^
	 -DPM_CMakeModules_PATH="%root%/externals/CMakeModules"^
	 -DCMAKEMODULES_PATH="%root%/externals/CMakeModules"^
	 -DPXSHARED_PATH="%root%/pxshared"^
	 && nmake
	
	rem copy libs/dlls to physx_lib folder for easier linking
	for /R "%~dp0" %%F in ("*.lib") do copy /Y %%~F . >NUL
	for /R "%~dp0" %%F in ("*.dll") do copy /Y %%~F . >NUL
popd

:skip_physx

rem Compile c wrapper lib
if not exist lib (
	mkdir lib
)

set libs=^
 physx_lib\PhysX_64.lib^
 physx_lib\PhysXCharacterKinematic_static_64.lib^
 physx_lib\PhysXCommon_64.lib^
 physx_lib\PhysXCooking_64.lib^
 physx_lib\PhysXExtensions_static_64.lib^
 physx_lib\PhysXFoundation_64.lib^
 physx_lib\PhysXPvdSDK_static_64.lib^
 physx_lib\PhysXVehicle_static_64.lib


cl.exe -Z7 /c /Folib\ -MP -DNDEBUG -IPhysx\physx\include -IPhysx\pxshared\include physx_lib.cpp
lib.exe lib\physx_lib.obj %libs% /out:.\lib\physx_lib.lib

rem Copy dlls
copy /Y physx_lib\PhysX_64.dll . >NUL
copy /Y physx_lib\PhysXCommon_64.dll . >NUL
copy /Y physx_lib\PhysXCooking_64.dll . >NUL
copy /Y physx_lib\PhysXFoundation_64.dll . >NUL
