toolchain="/home/danil_e71/toolchain/bin/aarch64-linux-android-"
output_path="./output"

export CROSS_COMPILE="/home/danil_e71/toolchain/bin/aarch64-linux-android-"

export ARCH=arm64
export TARGET_ARCH=arm64
export ARCH_MTK_PLATFORM=mt6795
export TARGET_BUILD_VARIANT=user
export MTK_TARGET_PROJECT=a55ml

build_kernel()
{ 
	if [ ! -d "$output_path" ]; then
	   mkdir "$output_path"
	fi

	echo "${output_path}"

	makeflags+=" O=${output_path}"

	#make clean

	make ${makeflags} a55ml_dtul_defconfig

	make ${makeflags} -j8 Image.gz-dtb
	#make ${makeflags} -j8 modules
}

build_kernel
