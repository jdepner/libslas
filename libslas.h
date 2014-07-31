
/********************************************************************************************

    libslas - A simple C LAS API for reading, creating, and updating LAS 1.2 files.
    Copyright (C) 2014 Jan Depner

    This library may be redistributed and/or modified under the terms of
    the GNU Lesser General Public License version 2.1, as published by the
    Free Software Foundation.  A copy of the LGPL 2.1 license is included with
    the LIBSLAS distribution and is available at: http://opensource.org/licenses/LGPL-2.1.
 
    This library is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
    PARTICULAR PURPOSE.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

    Please note that the file libslas_pd_functions.h is in the public domain and is NOT
    licensed under the LGPL.

********************************************************************************************/


/****************************************  IMPORTANT NOTE  **********************************

    Comments in this file that start with / * ! are being used by Doxygen to document the
    software.  Dashes in these comment blocks are used to create bullet lists.  The lack of
    blank lines after a block of dash preceeded comments means that the next block of dash
    preceeded comments is a new, indented bullet list.  I've tried to keep the Doxygen
    formatting to a minimum but there are some other items (like <br> and <pre>) that need
    to be left alone.  If you see a comment that starts with / * ! and there is something
    that looks a bit weird it is probably due to some arcane Doxygen syntax.  Be very
    careful modifying blocks of Doxygen comments.

*****************************************  IMPORTANT NOTE  **********************************/



#ifndef __LIBSLAS_H__
#define __LIBSLAS_H__

#ifdef  __cplusplus
extern "C" {
#endif


#ifndef DOXYGEN_SHOULD_SKIP_THIS


#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>



  /*  Preparing for language translation using GNU gettext at some point in the future.  */

#define _(String) (String)
#define N_(String) String

  /*
#include <libintl.h>
#define _(String) gettext (String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)
  */


#endif /*  DOXYGEN_SHOULD_SKIP_THIS  */


  /*! \mainpage LIBSLAS - A Simple LAS library

       <br><br>\section license License

       libslas - A simple C LAS API for reading, creating, and updating LAS files.<br>
       Copyright (C) 2014 Jan Depner

       This library may be redistributed and/or modified under the terms of
       the GNU Lesser General Public License version 2.1, as published by the
       Free Software Foundation.  A copy of the LGPL 2.1 license is included with
       the LIBSLAS distribution and is available at: http://opensource.org/licenses/LGPL-2.1.
 
       This library is distributed in the hope that it will be useful, but WITHOUT ANY
       WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
       PARTICULAR PURPOSE.

       You should have received a copy of the GNU Lesser General Public
       License along with this library; if not, write to the Free Software
       Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

       Please note that the file libslas_pd_functions.h is in the public domain and is NOT
       licensed under the LGPL.


       <br><br>\section intro Introduction

       Why did I write yet another LAS API?  Because I wanted something simpler than either liblas or LASlib.  This is what
       we call scratching your own itch.  Some of the design criteria for this API were:

       - KISS, Keep It Simple Stupid (I even thought about naming it KISSlas but I thought that might be misconstrued).
       - No Boost requirement
       - No GDAL requirement
       - No GeoTIFF requirement
       - Low level API written in C, not C++
       - No allocation/de-allocation of memory within the API
       - Use simple structures for LAS header, VLR header, and point data.
       - Handle Endianness in the API
       - Don't make the application deal with any bit fields in records.  In other words, the API handles packing and unpacking
         of bit fields
       - Make the API compute mins, maxes, and point counts when creating a file.
       - Compiles on Linux, MinGW32, and MinGW64
       - Direct access, non-sequential update of point data records (only certain fields - see LIBSLAS_POINT_DATA)
       - Thread safe (within reason).


       Due to my requirement for building on MinGW I was stuck using liblas 1.2.1 until I wrote this.  Liblas 1.2.1 appears to 
       be broken with respect to writing VLRs.  It also didn't write the global encoding field (although it would read it).
       The liblas people had no incentive to support MinGW so I decided to take things into my own hands.  It took an entire
       day to write and test this ;-)


       <br><br>\section caveats LIBSLAS API Caveats

       - Right now this API will create LAS 1.2 files and support read and update of LAS 1.1 and 1.2 files.  It will read and
         update LAS 1.0 files as well (I think) but it will hose the file marker and user bit fields.
       - If you want to create a LAS file you have to do it sequentially, that is, fill the header structure with the required
         fields (see LIBSLAS_HEADER), create the file, append your VLR records, and then append your point data records.  See example
         write code below.  On the bright side, you don't have to count your records, count points by return, compute your mins
         and maxes, or figure out the byte offset to the data.  The API does all this for you.
       - Will it work on big endian systems?  That's a definite maybe!  I haven't seen a big endian system in 15 years.  I tried
         to handle swapping but it may be incorrect.
       - I'm only trying to swap data for the 34735 and 34736 VLR records.  Those may be the only ones that require it.  At any
         rate, I don't have a big endian system to test it on.
       - This probably won't compile using the Microsoft C compiler or on Mac OS/X.  I've tried to deal with the fseeko64, fopen64
         things in libslas.c but I don't have either platform to build and test on.
       - I've left out anything that would allow you to build this as a DLL on Windows.  Dynamically loaded libraries are a
         giant PITA on Windows due to that __declspec nonsense.  You can build it as a shared library on Linux though ;-)
       - You can only open 64 LAS files simultaneously.  If that's a problem just go change LIBSLAS_MAX_FILES.
       - The LAS header and the VLR records are not modifiable after they have been written at this time (I don't really have a
         need for it).
       - You cannot add VLR records after the file has been created or after you have written point records (I don't have a need
         for that either).


       <br><br>\section read Example read code

       Here's a simple example of some pseudo-code to read a LAS file:


       <pre>

       int32_t main (int32_t argc, int32_t *argv[])
       {
         #
         #
         #

         LIBSLAS_HEADER                  header;
         LIBSLAS_VLR_HEADER              vlr_header;
         LIBSLAS_POINT_DATA              las;
         int32_t                         i, las_handle;

         #
         #
         #


         /#  Open the LAS file.  #/

         if ((las_handle = libslas_open_las_file (las_file, &header, LIBSLAS_READONLY)) < 0)
           {
             libslas_perror ();
             exit (-1);
           }


         /#  Print the LAS header to stderr.  #/

         libslas_dump_las_header (&header, stderr);


         /#  Get the VLRs  #/

         for (i = 0 ; i < (int32_t) header.number_of_VLRs ; i++)
           {
             if ((libslas_read_vlr_header (las_handle, i, &vlr_header)) < 0)
               {
                 libslas_perror ();
                 exit (-1);
               }


             /#  Print the VLR header to stderr.  #/

             libslas_dump_vlr_header (&vlr_header, stderr);


	     /#  Allocate memory for the data for this VLR.  We're doing this in the application to
	         avoid hidden memory allocation/deallocation in the API.  #/

             vlr_data = (uint8_t *) calloc (vlr_header.record_length_after_header, sizeof (uint8_t));
             if (vlr_data == NULL)
               {
                 perror ("Allocating VLR data memory");
                 exit (-1);
               }


	     /#  Read the data for this VLR.  #/

             if ((libslas_read_vlr_data (las_handle, i, vlr_data)) < 0)
               {
                 libslas_perror ();
                 exit (-1);
               }


             /#  Do something with the VLR information.  #/

             #
             #
             #


	     /#  Free the VLR data memory that you allocated above.  #/

	     free (vlr_data);
           }


         /#  Now read the point data records.  #/

         for (i = 0 ; i < (int32_t) header.number_of_point_records ; i++)
           {
             if (libslas_read_point_data (las_handle, i, &record))
               {
                 libslas_perror ();
                 exit (-1);
               }


             /#  Print the point data to stderr.  #/

             libslas_dump_point_data (&record, stderr);


             /#  Do something with the point data.  #/

             #
             #
             #
           }


         /#  Now close the file.  #/

         libslas_close_las_file (las_handle);

         return (0);
       }

       </pre><br><br>




       <br><br>\section write Example write code

       Here's a simple example of some pseudo-code to build a LAS file from some other file:


       <pre>

       int32_t main (int32_t argc, int32_t *argv[])
       {
         #
         #
         #

         LIBSLAS_HEADER                  header;
         LIBSLAS_VLR_HEADER              vlr_header;
         LIBSLAS_POINT_DATA              las;
         int32_t                         i, las_handle, pos;
         uint16_t                        var;

         #
         #
         #


         /#  Open the input file and get whatever information you need to have (like number of records).  #/

         #
         #
         #


         /#  Fill the header with the required fields (those marked with (c) in the structure definition in the
	     LIBSLAS_HEADER structure)  #/

         sprintf (header.system_id, "CZMIL - system %02d", system_number);

         sprintf (header.generating_software, "%s", software_name);

         header.point_data_format_id = 1;
         header.number_of_VLRs = 1;

         header.x_scale_factor = 0.0000001;
         header.y_scale_factor = 0.0000001;
         header.z_scale_factor = 0.001;

         header.x_offset = 0.0;
         header.y_offset = 0.0;
         header.z_offset = 0.0;


         /#  Set the global encoding slot in the header.  #/

         if (we want GPS time instead of GPS seconds of the week)
           {
             /#  Set it to 1 to store time as GPS time.  #/

             header.global_encoding = 1;
           }
         else
           {
             /#  Set it to 0 to store time as GPS seconds of the week.  #/

             header.global_encoding = 0;
           }


         header.file_creation_DOY = jday;
         header.file_creation_year = year;


         /#  Create the file and write the header.  #/

         if ((las_handle = libslas_create_las_file (las_name, &header)) < 0)
           {
             libslas_perror ();
             exit (-1);
           }


         /#  Set up the VLR record.  This is simple 34735 record (required).  #/

         /#  Fill the VLR header  #/

         sprintf (vlr_header.user_id, "LASF_Projection");
         vlr_header.record_id = 34735;
         vlr_header.record_length_after_header = 32;
         sprintf (vlr_header.description, "GeoTiff Projection Keys");


         /#  Now fill the VLR data.  #/

         pos = 0;


         /#  Key directory version  #/

         var = 1;
         memcpy (&vlr_data[pos], &var, 2); pos += 2;


         /#  Key revision  #/

         var = 1;
         memcpy (&vlr_data[pos], &var, 2); pos += 2;


         /#  Minor revision  #/

         var = 0;
         memcpy (&vlr_data[pos], &var, 2); pos += 2;


         /#  Number of keys  #/

         var = 3;
         memcpy (&vlr_data[pos], &var, 2); pos += 2;


         /#  You can look these up in the GeoTIFF spec (appendices) basically they mean...  #/

         /#  Key 1  #/

         /#  GTModelTypeGeoKey (1024)  #/

         var = 1024;
         memcpy (&vlr_data[pos], &var, 2); pos += 2;


         /#  Empty (in other words, no offset, we're putting the value here).  #/

         var = 0;
         memcpy (&vlr_data[pos], &var, 2); pos += 2;
         var = 1;
         memcpy (&vlr_data[pos], &var, 2); pos += 2;


         /#  ModelTypeGeographic  (2)   Geographic latitude-longitude System  #/

         var = 2;
         memcpy (&vlr_data[pos], &var, 2); pos += 2;


         /#  Key 2  #/

         /#  GeographicTypeGeoKey (2048)  #/

         var = 2048;
         memcpy (&vlr_data[pos], &var, 2); pos += 2;


         /#  Empty  (in other words, no offset, we're putting the value here). #/

         var = 0;
         memcpy (&vlr_data[pos], &var, 2); pos += 2;
         var = 1;
         memcpy (&vlr_data[pos], &var, 2); pos += 2;


         /#  GCS_WGS_84 (4326) or GCS_NAD_83 (4269)  #/

         if (our data is NAD83 instead of WGS84)
           {
             var = 4269;
           }
         else
          {
             var = 4326;
           }
         memcpy (&vlr_data[pos], &var, 2); pos += 2;


         /#  Key 3  #/

         /#  VerticalUnitsGeoKey (4099)  #/

         var = 4099;
         memcpy (&vlr_data[pos], &var, 2); pos += 2;


         /#  Empty  (in other words, no offset, we're putting the value here).  #/

         var = 0;
         memcpy (&vlr_data[pos], &var, 2); pos += 2;
         var = 1;
         memcpy (&vlr_data[pos], &var, 2); pos += 2;


         /#  Linear_Meter (9001)  #/

         var = 9001;
         memcpy (&vlr_data[pos], &var, 2); pos += 2;


         /#  Now append the VLR.  #/

         if (libslas_append_vlr_record (las_handle, &vlr_header, vlr_data))
           {
             libslas_perror ();
             exit (-1);
           }


         /#  Read throught the input file and append the point data records.  #/

         for (i = 0 ; i < number_of_records ; i++)
           {
             if (read_a_record (i) has an error)
               {
                 perror ("Error reading input record");
                 exit (-1);
               }


             /#  Fill the LAS record  #/

             las.edge_of_flightline = 0 or 1;
             las.scan_direction_flag = 0 or 1;

             las.x = X or longitude;
             las.y = Y or latitude;
             las.z = elevation;

             las.classification = 0 to 31;

             las.withheld = 0 or 1;
             las.key_point = 0 or 1;
             las.synthetic = 0 or 1;

             las.scan_angle_rank = -90 to 90;

             if (header.global_encoding)
               {
                 las.gps_time = GPS time;
               }
             else
               {
                 las.gps_time = GPS seconds of the week;
               }

             las.point_source_id = 0 to 65535;
             las.user_data = 0 to 255;

             las.return_number = 1 to 5;
             las.number_of_returns = 1 to 5;


             /#  Now, append the record.  #/

             if (libslas_append_point_data (las_handle, &las))
               {
                 libslas_perror ();
                 exit (-1);
               }
             }
           }


         /#  Close the files.  #/

         fclose (the input file);

         libslas_close_las_file (las_handle);

         return (0);
       }

       </pre><br><br>


       <br><br>\section update Example update code

       Here's a simple example of some pseudo-code to update a single record in a LAS file:


       <pre>

       int32_t main (int32_t argc, int32_t *argv[])
       {
         #
         #
         #

         LIBSLAS_HEADER                  header;
         LIBSLAS_POINT_DATA              las;
         int32_t                         i, las_handle;

         #
         #
         #


         /#  Open the LAS file.  #/

         if ((las_handle = libslas_open_las_file (las_file, &header, LIBSLAS_UPDATE)) < 0)
           {
             libslas_perror ();
             exit (-1);
           }


         /#  Read the record you want to modify (in this case record number 5,723,788).
	     Note: Record numbers start at 0.  #/

	 i = 5723788;
         if (libslas_read_point_data (las_handle, i, &record))
           {
             libslas_perror ();
             exit (-1);
           }


         /#  Now modify something (in this case we'll set the record to "withheld").  #/

         record.withheld = 1;


         /#  Update the record.  #/

         if (libslas_update_point_data (las_handle, i, &record))
           {
             libslas_perror ();
             exit (-1);
           }


         libslas_close_las_file (las_handle);

         return (0);
       }

       </pre><br><br>


       <br><br>\section threads Thread Safety

       The LIBSLAS I/O library is thread safe if you follow some simple rules.  First, it is only thread safe if you use a unique
       LIBSLAS file handle for each thread.  Also, the libslas_create_las_file, libslas_open_las_file, and libslas_close_las_file
       functions are not thread safe due to the fact that they assign or clear the LIBSLAS file handles.  In order to use the
       library in multiple threads you must open (or create) the LAS files prior to starting the threads and close them after the
       threads have completed.  Some common sense must be brought to bear when trying to create a multithreaded program that works
       with LAS files.  When you are creating LAS files, threads should only work with one file.  So, for example, if you want to
       create 16 LAS files from 16 sets of input data you would create all 16 new LAS files using libslas_create_las_file prior to
       starting your threads.  After that, each thread would append records to each of the files.  When the threads are finished
       you would use libslas_close_las_file to close each of the new LAS files.

       If you want multiple threads to read from multiple or single LAS files at the same time you must open each file for each
       thread to get a separate LIBSLAS file handle for each file and thread.  The static data in the LIBSLAS API is segregated by the 
       LIBSLAS file handle so there should be no collision problems.  Again, you have to open the files prior to starting your threads
       and close them after the threads are finished.


       <br><br>\section notes Documentation Notes

       The easiest way to find documentation for a particular public C function is by looking for it in the Doxygen documentation
       for the libslas.h file.  All of the publicly accessible functions are defined there.  For the application programmer there
       should be nothing much else of interest (assuming everything is working properly).

       For anyone wanting to work on the API itself, the libslas.c file is where the action is.  If you would like to see a
       history of the changes that have been made to the LIBSLAS API you can look at libslas_version.h.

       <br><br>

  */


#define       LIBSLAS_MAX_FILES                        64        /*!<  Maximum number of open LAS files.  */


#define       LIBSLAS_HEADER_SIZE                      227       /*!<  The header size is always 227 bytes for 1.0, 1.1, and 1.2  */


  /*  File open modes.  */

#define       LIBSLAS_UPDATE                           0         /*!<  Open file for update.  */
#define       LIBSLAS_READONLY                         1         /*!<  Open file for read only.  */


  /*  Error conditions.  */

#define       LIBSLAS_SUCCESS                          0
#define       LIBSLAS_APPEND_ERROR                     -1
#define       LIBSLAS_CLOSE_ERROR                      -2
#define       LIBSLAS_HEADER_READ_ERROR                -3
#define       LIBSLAS_HEADER_READ_FSEEK_ERROR          -4
#define       LIBSLAS_HEADER_WRITE_ERROR               -5
#define       LIBSLAS_HEADER_WRITE_FSEEK_ERROR         -6
#define       LIBSLAS_INCORRECT_VERSION_ERROR          -7
#define       LIBSLAS_INVALID_FILENAME_ERROR           -8
#define       LIBSLAS_INVALID_GLOBAL_ENCODING_ERROR    -9
#define       LIBSLAS_INVALID_MODE_ERROR               -10
#define       LIBSLAS_INVALID_POINT_FORMAT_ID_ERROR    -11
#define       LIBSLAS_INVALID_RECORD_NUMBER_ERROR      -12
#define       LIBSLAS_INVALID_VLR_RECORD_NUMBER_ERROR  -13
#define       LIBSLAS_LAS_CREATE_ERROR                 -14
#define       LIBSLAS_NOT_LAS_FILE_ERROR               -15
#define       LIBSLAS_NOT_OPEN_FOR_UPDATE_ERROR        -16
#define       LIBSLAS_OPEN_READONLY_ERROR              -17
#define       LIBSLAS_OPEN_UPDATE_ERROR                -18
#define       LIBSLAS_READ_ERROR                       -19
#define       LIBSLAS_READ_FSEEK_ERROR                 -20
#define       LIBSLAS_RETURN_NUMBER_OUT_OF_RANGE_ERROR -21
#define       LIBSLAS_TOO_MANY_OPEN_FILES_ERROR        -22
#define       LIBSLAS_UPDATE_FSEEK_ERROR               -23
#define       LIBSLAS_UPDATE_READ_ERROR                -24
#define       LIBSLAS_VLR_APPEND_ERROR                 -25
#define       LIBSLAS_VLR_READ_ERROR                   -26
#define       LIBSLAS_VLR_READ_FSEEK_ERROR             -27
#define       LIBSLAS_VLR_WRITE_ERROR                  -28
#define       LIBSLAS_VLR_WRITE_FSEEK_ERROR            -29
#define       LIBSLAS_WRITE_ERROR                      -30


  /*!

      - LIBSLAS header structure.  Header key definitions are as follows:

          - (a) = Set by the API at creation time or later (e.g. version_major, number_of_point_records, max_x)
          - (c) = Defined by the application program only at creation time (e.g. point_data_format_id)

  */

  typedef struct 
  {                                                                 /*   Key             Definition  */
    uint16_t                    file_source_id;                     /*!< (c)             See ASPRS LAS specification  */
    uint16_t                    global_encoding;                    /*!< (c)             See ASPRS LAS specification  */
    uint32_t                    GUID_data_1;                        /*!< (c)             See ASPRS LAS specification  */
    uint16_t                    GUID_data_2;                        /*!< (c)             See ASPRS LAS specification  */
    uint16_t                    GUID_data_3;                        /*!< (c)             See ASPRS LAS specification  */
    char                        GUID_data_4[9];                     /*!< (c)             See ASPRS LAS specification  */
    uint8_t                     version_major;                      /*!< (a)             See ASPRS LAS specification  */
    uint8_t                     version_minor;                      /*!< (a)             See ASPRS LAS specification  */
    char                        system_id[33];                      /*!< (c)             See ASPRS LAS specification  */
    char                        generating_software[33];            /*!< (c)             See ASPRS LAS specification  */
    uint16_t                    file_creation_DOY;                  /*!< (c)             See ASPRS LAS specification  */
    uint16_t                    file_creation_year;                 /*!< (c)             See ASPRS LAS specification  */
    uint16_t                    header_size;                        /*!< (a)             See ASPRS LAS specification  */
    uint32_t                    offset_to_point_data;               /*!< (a)             See ASPRS LAS specification  */
    uint32_t                    number_of_VLRs;                     /*!< (c)             See ASPRS LAS specification  */
    uint8_t                     point_data_format_id;               /*!< (c)             See ASPRS LAS specification  */
    uint16_t                    point_data_record_length;           /*!< (a)             See ASPRS LAS specification  */
    uint32_t                    number_of_point_records;            /*!< (a)             See ASPRS LAS specification  */
    uint32_t                    number_of_points_by_return[5];      /*!< (a)             See ASPRS LAS specification  */
    double                      x_scale_factor;                     /*!< (c)             See ASPRS LAS specification  */
    double                      y_scale_factor;                     /*!< (c)             See ASPRS LAS specification  */
    double                      z_scale_factor;                     /*!< (c)             See ASPRS LAS specification  */
    double                      x_offset;                           /*!< (c)             See ASPRS LAS specification  */
    double                      y_offset;                           /*!< (c)             See ASPRS LAS specification  */
    double                      z_offset;                           /*!< (c)             See ASPRS LAS specification  */
    double                      max_x;                              /*!< (a)             See ASPRS LAS specification  */
    double                      min_x;                              /*!< (a)             See ASPRS LAS specification  */
    double                      max_y;                              /*!< (a)             See ASPRS LAS specification  */
    double                      min_y;                              /*!< (a)             See ASPRS LAS specification  */
    double                      max_z;                              /*!< (a)             See ASPRS LAS specification  */
    double                      min_z;                              /*!< (a)             See ASPRS LAS specification  */
  } LIBSLAS_HEADER;



  /*!

      - LIBSLAS Variable Length Record (VLR) header structure.  Header key definitions are as follows:

          - (a) = Set by the API at creation time or later (e.g. reserved)
          - (c) = Defined by the application program only at creation time (e.g. user_id)

  */

  typedef struct 
  {                                                                 /*   Key             Definition  */
    uint16_t                    reserved;                           /*!< (a)             See ASPRS LAS specification  */
    char                        user_id[17];                        /*!< (c)             See ASPRS LAS specification  */
    uint16_t                    record_id;                          /*!< (c)             See ASPRS LAS specification  */
    uint16_t                    record_length_after_header;         /*!< (c)             See ASPRS LAS specification  */
    char                        description[33];                    /*!< (c)             See ASPRS LAS specification  */
  } LIBSLAS_VLR_HEADER;


  /*!

      - LIBSLAS Point Data structure.  Key definitions are as follows:

          - (c) = Defined by the application program at creation time.
          - (m) = Modifiable by the application program using the libslas_update_point_data function.
  */

  typedef struct
  {                                                                 /*   Key             Definition  */
    double                      x;                                  /*!< (c)             Double precision float X value (DO NOT scale or offset this value when
                                                                                         writing, it will be scaled and offset by the API)  */
    double                      y;                                  /*!< (c)             Double precision float Y value (DO NOT scale or offset this value when
                                                                                         writing, it will be scaled and offset by the API)  */
    float                       z;                                  /*!< (c)             Single precision float Z value (DO NOT scale or offset this value when
                                                                                         writing, it will be scaled and offset by the API)  */
    uint16_t                    intensity;                          /*!< (c)             See ASPRS LAS specification  */
    uint8_t                     return_number;                      /*!< (c)             Return number of this return (1-5)  */
    uint8_t                     number_of_returns;                  /*!< (c)             Number of returns for this pulse (1-5)  */
    uint8_t                     scan_direction_flag;                /*!< (c)             0 = negative scan direction, 1 = positive scan direction  */
    uint8_t                     edge_of_flightline;                 /*!< (c)             1 = edge of flightline, 0 = not edge of flightline  */
    uint8_t                     classification;                     /*!< (m)             5 bit classification (0-31)  */
    uint8_t                     withheld;                           /*!< (m)             1 if withheld bit is set  */
    uint8_t                     key_point;                          /*!< (m)             1 if key point bit is set  */
    uint8_t                     synthetic;                          /*!< (m)             1 if synthetic bit is set  */
    int8_t                      scan_angle_rank;                    /*!< (c)             See ASPRS LAS specification  */
    uint8_t                     user_data;                          /*!< (m)             See ASPRS LAS specification  */
    uint16_t                    point_source_id;                    /*!< (m)             See ASPRS LAS specification  */
    double                      gps_time;                           /*!< (c)             See ASPRS LAS specification  */
    uint16_t                    red;                                /*!< (m)             See ASPRS LAS specification  */
    uint16_t                    green;                              /*!< (m)             See ASPRS LAS specification  */
    uint16_t                    blue;                               /*!< (m)             See ASPRS LAS specification  */
  } LIBSLAS_POINT_DATA;



  /*!  LIBSLAS Public function declarations.  */

  int32_t libslas_open_las_file (char *path, LIBSLAS_HEADER *header, int32_t mode);
  int32_t libslas_create_las_file (char *path, LIBSLAS_HEADER *header);
  int32_t libslas_close_las_file (int32_t hnd);
  int32_t libslas_read_vlr_header (int32_t hnd, int32_t recnum, LIBSLAS_VLR_HEADER *vlr_header);
  int32_t libslas_read_vlr_data (int32_t hnd, int32_t recnum, uint8_t *vlr_data);
  int32_t libslas_append_vlr_record (int32_t hnd, LIBSLAS_VLR_HEADER *vlr_header, uint8_t *vlr_data);
  int32_t libslas_read_point_data (int32_t hnd, int32_t recnum, LIBSLAS_POINT_DATA *record);
  int32_t libslas_append_point_data (int32_t hnd, LIBSLAS_POINT_DATA *record);
  int32_t libslas_update_point_data (int32_t hnd, int32_t recnum, LIBSLAS_POINT_DATA *record);

  char *libslas_strerror ();
  void libslas_perror ();
  char *libslas_get_version ();

  void libslas_dump_las_header (LIBSLAS_HEADER *header, FILE *fp);
  void libslas_dump_vlr_header (LIBSLAS_VLR_HEADER *vlr_header, FILE *fp);
  void libslas_dump_point_data (LIBSLAS_POINT_DATA *record, FILE *fp);


#ifdef  __cplusplus
}
#endif


#endif  /*  __LIBSLAS_H__  */
