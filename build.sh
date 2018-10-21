toolchain="/home/danil_e71/aarch64-linux-android-4.9/bin/aarch64-linux-android-"
output_path="./output"

export CROSS_COMPILE=$toolchain

export ARCH=arm64
export TARGET_ARCH=arm64
export ARCH_MTK_PLATFORM=mt6795

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
}

build_kernel
