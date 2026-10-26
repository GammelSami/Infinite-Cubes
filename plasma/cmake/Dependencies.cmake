include(FetchContent)
set(PLASMA_DOWNLOAD_CACHE "" CACHE PATH "Optional directory of pinned source archives")
function(plasma_archive output filename remote)
 if(PLASMA_DOWNLOAD_CACHE AND EXISTS "${PLASMA_DOWNLOAD_CACHE}/${filename}")
  set(${output} "${PLASMA_DOWNLOAD_CACHE}/${filename}" PARENT_SCOPE)
 else()
  set(${output} "${remote}" PARENT_SCOPE)
 endif()
endfunction()
plasma_archive(ZLIB_URL zlib-1.3.1.tar.gz https://zlib.net/fossils/zlib-1.3.1.tar.gz)
plasma_archive(GLEW_URL glew-2.2.0.tgz https://downloads.sourceforge.net/glew/glew-2.2.0.tgz)
set(ZLIB_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(zlib URL "${ZLIB_URL}" TIMEOUT 120 INACTIVITY_TIMEOUT 30 URL_HASH SHA256=9a93b2b7dfdac77ceba5a558a580e74667dd6fede4585b91eefb60f03b72df23)
FetchContent_MakeAvailable(zlib)
set_target_properties(zlib PROPERTIES EXCLUDE_FROM_ALL TRUE)
target_compile_definitions(zlibstatic PUBLIC Z_PREFIX)
target_compile_definitions(zlibstatic PRIVATE z_errmsg=plasma_z_errmsg)
FetchContent_Declare(glew_source URL "${GLEW_URL}" TIMEOUT 120 INACTIVITY_TIMEOUT 30 URL_HASH SHA256=d4fc82893cfb00109578d0a1a2337fb8ca335b3ceccf97b97e5cc7f08e4353e1)
FetchContent_GetProperties(glew_source)
if(NOT glew_source_POPULATED)
 FetchContent_Populate(glew_source)
endif()
add_library(glew STATIC ${glew_source_SOURCE_DIR}/src/glew.c)
target_include_directories(glew PUBLIC ${glew_source_SOURCE_DIR}/include)
target_compile_definitions(glew PUBLIC GLEW_STATIC)
include(${CMAKE_CURRENT_LIST_DIR}/FreeImage.cmake)
add_library(lcms STATIC
 vendor/lcms2-2.0/src/cmscam02.c
 vendor/lcms2-2.0/src/cmscgats.c
 vendor/lcms2-2.0/src/cmscnvrt.c
 vendor/lcms2-2.0/src/cmserr.c
 vendor/lcms2-2.0/src/cmsgamma.c
 vendor/lcms2-2.0/src/cmsgmt.c
 vendor/lcms2-2.0/src/cmsintrp.c
 vendor/lcms2-2.0/src/cmsio0.c
 vendor/lcms2-2.0/src/cmsio1.c
 vendor/lcms2-2.0/src/cmslut.c
 vendor/lcms2-2.0/src/cmsmd5.c
 vendor/lcms2-2.0/src/cmsmtrx.c
 vendor/lcms2-2.0/src/cmsnamed.c
 vendor/lcms2-2.0/src/cmsopt.c
 vendor/lcms2-2.0/src/cmspack.c
 vendor/lcms2-2.0/src/cmspcs.c
 vendor/lcms2-2.0/src/cmsplugin.c
 vendor/lcms2-2.0/src/cmsps2.c
 vendor/lcms2-2.0/src/cmssamp.c
 vendor/lcms2-2.0/src/cmssm.c
 vendor/lcms2-2.0/src/cmstypes.c
 vendor/lcms2-2.0/src/cmsvirt.c
 vendor/lcms2-2.0/src/cmswtpnt.c
 vendor/lcms2-2.0/src/cmsxform.c
)
target_include_directories(lcms PUBLIC vendor/lcms2-2.0/include)
add_library(jpeg STATIC
 vendor/libjpeg-8b/jcapimin.c
 vendor/libjpeg-8b/jcparam.c
 vendor/libjpeg-8b/jcapistd.c
 vendor/libjpeg-8b/jdatadst.c
 vendor/libjpeg-8b/jerror.c
 vendor/libjpeg-8b/jutils.c
 vendor/libjpeg-8b/jmemmgr.c
 vendor/libjpeg-8b/jcomapi.c
 vendor/libjpeg-8b/jcmarker.c
 vendor/libjpeg-8b/jcinit.c
 vendor/libjpeg-8b/jmemnobs.c
 vendor/libjpeg-8b/jcmainct.c
 vendor/libjpeg-8b/jccoefct.c
 vendor/libjpeg-8b/jchuff.c
 vendor/libjpeg-8b/jcarith.c
 vendor/libjpeg-8b/jcdctmgr.c
 vendor/libjpeg-8b/jcprepct.c
 vendor/libjpeg-8b/jcsample.c
 vendor/libjpeg-8b/jccolor.c
 vendor/libjpeg-8b/jcmaster.c
 vendor/libjpeg-8b/jfdctflt.c
 vendor/libjpeg-8b/jfdctfst.c
 vendor/libjpeg-8b/jfdctint.c
 vendor/libjpeg-8b/jaricom.c
)
target_include_directories(jpeg PUBLIC vendor/libjpeg-8b/.)
add_library(freetype STATIC
 vendor/freetype-2.4.1/src/base/ftbase.c
 vendor/freetype-2.4.1/src/base/ftinit.c
 vendor/freetype-2.4.1/src/base/ftsystem.c
 vendor/freetype-2.4.1/src/bdf/bdf.c
 vendor/freetype-2.4.1/src/smooth/smooth.c
 vendor/freetype-2.4.1/src/sfnt/sfnt.c
 vendor/freetype-2.4.1/src/raster/raster.c
 vendor/freetype-2.4.1/src/pshinter/pshinter.c
 vendor/freetype-2.4.1/src/psnames/psmodule.c
 vendor/freetype-2.4.1/src/psaux/psaux.c
 vendor/freetype-2.4.1/src/pcf/pcf.c
 vendor/freetype-2.4.1/src/winfonts/winfnt.c
 vendor/freetype-2.4.1/src/type42/type42.c
 vendor/freetype-2.4.1/src/pfr/pfr.c
 vendor/freetype-2.4.1/src/cid/type1cid.c
 vendor/freetype-2.4.1/src/cff/cff.c
 vendor/freetype-2.4.1/src/type1/type1.c
 vendor/freetype-2.4.1/src/truetype/truetype.c
 vendor/freetype-2.4.1/src/autofit/autofit.c
 vendor/freetype-2.4.1/src/lzw/ftlzw.c
 vendor/freetype-2.4.1/src/gzip/ftgzip.c
 vendor/freetype-2.4.1/src/base/ftglyph.c
 vendor/freetype-2.4.1/src/base/ftstroke.c
 vendor/freetype-2.4.1/src/base/ftbitmap.c
)
target_include_directories(freetype PUBLIC vendor/freetype-2.4.1/include)
target_compile_definitions(freetype PRIVATE FT2_BUILD_LIBRARY)
add_library(pugixml STATIC
 vendor/pugixml-0.9/src/pugixml.cpp
)
target_include_directories(pugixml PUBLIC vendor/pugixml-0.9/src)

# Keep the editor's JPEG 8 API separate from FreeImage's bundled JPEG 9.
target_compile_definitions(jpeg PUBLIC
 jcopy_block_row=plasma_jcopy_block_row
 jcopy_sample_rows=plasma_jcopy_sample_rows
 jdiv_round_up=plasma_jdiv_round_up
 jinit_arith_encoder=plasma_jinit_arith_encoder
 jinit_c_coef_controller=plasma_jinit_c_coef_controller
 jinit_c_main_controller=plasma_jinit_c_main_controller
 jinit_c_master_control=plasma_jinit_c_master_control
 jinit_c_prep_controller=plasma_jinit_c_prep_controller
 jinit_color_converter=plasma_jinit_color_converter
 jinit_compress_master=plasma_jinit_compress_master
 jinit_downsampler=plasma_jinit_downsampler
 jinit_forward_dct=plasma_jinit_forward_dct
 jinit_huff_encoder=plasma_jinit_huff_encoder
 jinit_marker_writer=plasma_jinit_marker_writer
 jinit_memory_mgr=plasma_jinit_memory_mgr
 jpeg_CreateCompress=plasma_jpeg_CreateCompress
 jpeg_abort=plasma_jpeg_abort
 jpeg_abort_compress=plasma_jpeg_abort_compress
 jpeg_add_quant_table=plasma_jpeg_add_quant_table
 jpeg_alloc_huff_table=plasma_jpeg_alloc_huff_table
 jpeg_alloc_quant_table=plasma_jpeg_alloc_quant_table
 jpeg_aritab=plasma_jpeg_aritab
 jpeg_calc_jpeg_dimensions=plasma_jpeg_calc_jpeg_dimensions
 jpeg_default_colorspace=plasma_jpeg_default_colorspace
 jpeg_default_qtables=plasma_jpeg_default_qtables
 jpeg_destroy=plasma_jpeg_destroy
 jpeg_destroy_compress=plasma_jpeg_destroy_compress
 jpeg_fdct_10x10=plasma_jpeg_fdct_10x10
 jpeg_fdct_10x5=plasma_jpeg_fdct_10x5
 jpeg_fdct_11x11=plasma_jpeg_fdct_11x11
 jpeg_fdct_12x12=plasma_jpeg_fdct_12x12
 jpeg_fdct_12x6=plasma_jpeg_fdct_12x6
 jpeg_fdct_13x13=plasma_jpeg_fdct_13x13
 jpeg_fdct_14x14=plasma_jpeg_fdct_14x14
 jpeg_fdct_14x7=plasma_jpeg_fdct_14x7
 jpeg_fdct_15x15=plasma_jpeg_fdct_15x15
 jpeg_fdct_16x16=plasma_jpeg_fdct_16x16
 jpeg_fdct_16x8=plasma_jpeg_fdct_16x8
 jpeg_fdct_1x1=plasma_jpeg_fdct_1x1
 jpeg_fdct_1x2=plasma_jpeg_fdct_1x2
 jpeg_fdct_2x1=plasma_jpeg_fdct_2x1
 jpeg_fdct_2x2=plasma_jpeg_fdct_2x2
 jpeg_fdct_2x4=plasma_jpeg_fdct_2x4
 jpeg_fdct_3x3=plasma_jpeg_fdct_3x3
 jpeg_fdct_3x6=plasma_jpeg_fdct_3x6
 jpeg_fdct_4x2=plasma_jpeg_fdct_4x2
 jpeg_fdct_4x4=plasma_jpeg_fdct_4x4
 jpeg_fdct_4x8=plasma_jpeg_fdct_4x8
 jpeg_fdct_5x10=plasma_jpeg_fdct_5x10
 jpeg_fdct_5x5=plasma_jpeg_fdct_5x5
 jpeg_fdct_6x12=plasma_jpeg_fdct_6x12
 jpeg_fdct_6x3=plasma_jpeg_fdct_6x3
 jpeg_fdct_6x6=plasma_jpeg_fdct_6x6
 jpeg_fdct_7x14=plasma_jpeg_fdct_7x14
 jpeg_fdct_7x7=plasma_jpeg_fdct_7x7
 jpeg_fdct_8x16=plasma_jpeg_fdct_8x16
 jpeg_fdct_8x4=plasma_jpeg_fdct_8x4
 jpeg_fdct_9x9=plasma_jpeg_fdct_9x9
 jpeg_fdct_float=plasma_jpeg_fdct_float
 jpeg_fdct_ifast=plasma_jpeg_fdct_ifast
 jpeg_fdct_islow=plasma_jpeg_fdct_islow
 jpeg_finish_compress=plasma_jpeg_finish_compress
 jpeg_free_large=plasma_jpeg_free_large
 jpeg_free_small=plasma_jpeg_free_small
 jpeg_get_large=plasma_jpeg_get_large
 jpeg_get_small=plasma_jpeg_get_small
 jpeg_mem_available=plasma_jpeg_mem_available
 jpeg_mem_dest=plasma_jpeg_mem_dest
 jpeg_mem_init=plasma_jpeg_mem_init
 jpeg_mem_term=plasma_jpeg_mem_term
 jpeg_natural_order=plasma_jpeg_natural_order
 jpeg_natural_order2=plasma_jpeg_natural_order2
 jpeg_natural_order3=plasma_jpeg_natural_order3
 jpeg_natural_order4=plasma_jpeg_natural_order4
 jpeg_natural_order5=plasma_jpeg_natural_order5
 jpeg_natural_order6=plasma_jpeg_natural_order6
 jpeg_natural_order7=plasma_jpeg_natural_order7
 jpeg_open_backing_store=plasma_jpeg_open_backing_store
 jpeg_quality_scaling=plasma_jpeg_quality_scaling
 jpeg_set_colorspace=plasma_jpeg_set_colorspace
 jpeg_set_defaults=plasma_jpeg_set_defaults
 jpeg_set_linear_quality=plasma_jpeg_set_linear_quality
 jpeg_set_quality=plasma_jpeg_set_quality
 jpeg_simple_progression=plasma_jpeg_simple_progression
 jpeg_start_compress=plasma_jpeg_start_compress
 jpeg_std_error=plasma_jpeg_std_error
 jpeg_std_message_table=plasma_jpeg_std_message_table
 jpeg_stdio_dest=plasma_jpeg_stdio_dest
 jpeg_suppress_tables=plasma_jpeg_suppress_tables
 jpeg_write_m_byte=plasma_jpeg_write_m_byte
 jpeg_write_m_header=plasma_jpeg_write_m_header
 jpeg_write_marker=plasma_jpeg_write_marker
 jpeg_write_raw_data=plasma_jpeg_write_raw_data
 jpeg_write_scanlines=plasma_jpeg_write_scanlines
 jpeg_write_tables=plasma_jpeg_write_tables
 jround_up=plasma_jround_up
 jzero_far=plasma_jzero_far
)

if(MSVC)
 target_compile_options(lcms PRIVATE "/FI${CMAKE_CURRENT_SOURCE_DIR}/include/LcmsSymbols.h")
else()
 target_compile_options(lcms PRIVATE -include "${CMAKE_CURRENT_SOURCE_DIR}/include/LcmsSymbols.h")
endif()
