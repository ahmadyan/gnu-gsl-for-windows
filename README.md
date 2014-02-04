gnu-gsl-for-windows
===================

Fork of Brian Gladman's Gnu GSL package for windows, version 1.16


# Building the GNU Scientific Library with Microsoft Visual Sudio 2012

This is GSL, the GNU Scientific Library, a collection of numerical 
routines for scientific computing.  Please see the readme.txt file 
for futher information on GNU and GSL.  This file only describes 
how this version is configured.

The following sections describe how to compile GSL using Microsoft 
Visual Studio 2012.  

Although The files are designed for use with the full versions of 
Visual Studio 2012, they can be used with Microsoft Visual C++ 
Express 2012 and/or the Windows 7.1 SDK.  To run the GSL tests, 
Python also needs to be installed (2.6 or later).

## Set Up Directories and Files


First the GSL zip file for version 1.16 needs to be obtained and 
expanded into a directory tree under a an  appropriate root directory. 
This direcrtory will be assumed to be 'gsl-1.16' here.  Next, the zip 
file gsl-1.16.vc11.zip needs to be expanded so that its directories 
and files are in the gsl-1.16 directory - this should add a 
'build.vc11' directory at the top level inside your GSL root directory. 
It will also overwrite a number of GSL files that need to be changed 
(see the text file gsl.vc11.changes.txt).

The GSL solution files gsl.lib.sln or gsl.dll.sln can then be opened 
to build GSL either as a static library or a dynamic link library 
respectively.  The projects in these solutions are:

Utilities build:

    gsldefs    - a utility that generates GSL and CBLAS DEF files
    gslhdrs    - a utility that adds DLL import/export directives
                 to the GSL and CBLAS header files.
                 
GSL build:

    gsldll     - builds the GSL dynamic link library
    cblasdll   - builds the BLAS dynamic link library
	
    gsllib     - builds the GSL static library (multi-threaded)
    cblaslib   - builds the BLAS static library (multi-threaded)

In addition there are also a large number of test projects that
will be described later.

## Building GSL


### Copying or Modifying GSL Header Files


Either the 'gsldefs' or the 'gslhdrs' project has to be built
BEFORE any GSL libraries are built. Do NOT run both --  gslhdrs
is preferred.  This step only needs to be done ONCE.

After either gslhdrs or gsldefs is built, they will then be run
automatically and will copy all the GSL header files into the
gsl subdirectory within the GSL root directory.  You should now
have the following directory layout:

   gsl-1.16       - the GSL root directory
     blas       )
     block      )  - GSL subdirectories 
     bspline    )
     build.vc11    - the Visual Studio build files
     ....          - more GSL subdirectories 
     gsl           - should now contain the GSL header files
     ....

The follwoing two sections describe what gslhdrs and gsldefs programs
do - you can skip these if you have completed this step successfully.
  
### GSLHDRS


The gslhdrs.exe executable collects all the headers into the gsl 
sub-directory in a modified form:

  (a) it adds the symbols GSL_FUN (for gsl functions) or CBL_FUN 
      (for cblas functions) before each GSL or CBLAS function 
      definition;
      
  (b) it then inserts the following text into each header so that
      so that the symbols GSL_FUN and CBL_FUN can be used to 
      declare GSL and CBLAS symbols for static library, DLL import 
      or DLL export depending on what is being built at the time.

In GSL headers:

    if !defined( GSL_FUN )
      if !defined( GSL_DLL )
        define GSL_FUN extern
      elif defined( BUILD_GSL_DLL )
        define GSL_FUN extern __declspec(dllexport)
      else
        define GSL_FUN extern __declspec(dllimport)
      endif
    endif

In CBLAs headers (gsl_cblas.h):

   if !defined( CBL_FUN )
      if !defined( CBLAS_DLL )
        define CBL_FUN extern
      elif defined( BUILD_CBLAS_DLL )
        define CBL_FUN extern __declspec(dllexport)
      else
        define CBL_FUN extern __declspec(dllimport)
      endif
    endif

### GSLDEFS

The project gsldefs can be used as an ALTERNATIVE to gslhdrs. This 
copies the header files as before but then creates DEF files that 
can be added to the DLL projects to create the exports needed. This
avoids the need to modify the header files. If this approach is
used the DEF files gsl.def and cblas.def need to be added to the 
respective DLL projects manually as the default build approach is 
based on the use of gslhdrs.

## Static Library Build

At this point the two library projects can be built. The resulting
libraries are placed in appropriate subdirectories within the
gsl-1.16 directory as follows:

	gsl-1.16
	  lib
      win32
        release
	      debug
	    x64
	      release
	      debug

## DLL Build

The VC++ DLL build projects can be built as required. Note, however,
that either gslhdrs.exe or gsldefs.exe must be run first as described
esrlier.  The resulting DLLs and libraries are placed in appropriate 
subdirectories within the gsl-1.16 directory:

	gsl-1.16
	  dll
		  win32
			  release
			  debug
		  x64
			  release
			  debug

## Testing


The GSL solution files test.gsl.lib.sln or test.gsl.dll.sln can be used
to test the GSL static or DLL libraries.  The configuration set for these
projects has to be the same as that used to build the GSL libraries.
 
Each of the test projects runs and puts its output in the appropriate
subdirectory:

	gsl-1.16
	  <lib|dll>
		  win32
			  release
			  debug
		  x64
			  release
			  debug
 
as a name.out file where 'name' indicates the test in question. These 
output files will contain failure messages if there are any problems.

If you have Python installed you can run the static library or the DLL
tests (after the tests have been built in Visual Studio by running the 
programs run_tests.py found in the libtests and dlltests directories 
respectively.   

## NOTE

The DLL projects can be built using either method but the method using 
gslhdrs, although more complex, allows the direct import of DLL symbols
and direct static and dynamic linking.

# GSL Use in Windows Applications.


The GSL and CBLAS static libraries can be used in a straightforward way 
with Windows Applications. These are multi threaded since single threaded 
libraries are no longer available for VC++.

The GSL and CBLAS DLLs are fairly easy to use but there are a few issues to 
watch out for.  Firstly the application must be able to locate the DLLs so 
they must be on the DLL search path or in the same directory as the 
application. Secondly it is necessary to link to the lib files in the dll 
folder rather than those in the lib folder. Thirdly GSL_DLL must be defined 
to tell the application that a DLL is being used.

The GSL build projects create GSL with range checking turned off. If you
wnat it turned on you should add the symbol GSL_RANGE_CHECK to the lists
of pre-processor defines in the cbaslib and gsllib build projects.  

   Brian Gladman, Worcester UK, July 2013