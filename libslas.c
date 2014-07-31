
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


/*  Try to handle some things for MSC and Mac OS/X.  Does it work?  Who knows, I don't have
    either platform.  */

#if (defined _WIN32) && (defined _MSC_VER)
#undef fseeko64
#undef ftello64
#undef fopen64
#define fseeko64(x, y, z) _fseeki64((x), (y), (z))
#define ftello64(x)       _ftelli64((x))
#define fopen64(x, y)      fopen((x), (y))
#elif defined(__APPLE__)
#define fseeko64(x, y, z) fseek((x), (y), (z))
#define ftello64(x)       ftell((x))
#define fopen64(x, y)     fopen((x), (y))
#endif


#include "libslas.h"
#include "libslas_version.h"


#undef LIBSLAS_DEBUG
#define LIBSLAS_DEBUG_OUTPUT stderr

#ifndef NINT64
  #define NINT64(a)   ((a)<0.0 ? (int64_t) ((a) - 0.5) : (int64_t) ((a) + 0.5))
#endif

#ifndef MAX
  #define MAX(x,y)      (((x) > (y)) ? (x) : (y))
#endif

#ifndef MIN
  #define MIN(x,y)      (((x) < (y)) ? (x) : (y))
#endif


/*  The POINT DATA FORMAT 3 (largest record) record size is always 34 bytes.  */

#define POINT_DATA_SIZE 34


/*!  This is the structure we use to keep track of important formatting data for an open LAS file.  */

typedef struct
{
  char              path[1024];                 /*!<  Fully qualified LAS file name.  */
  FILE              *fp;                        /*!<  LAS file pointer.  */
  LIBSLAS_HEADER    header;                     /*!<  LAS file header.  */
  uint8_t           swap;                       /*!<  Set to 1 on big endian systems.  */
  uint8_t           at_end;                     /*!<  Set if the LAS file position is at the end of the file.  */
  uint8_t           created;                    /*!<  Set if we created the LAS file.  */
  uint8_t           modified;                   /*!<  Set if the LAS file header has been modified.  */
  uint8_t           write;                      /*!<  Set if the last action to the LAS file was a write.  */
  int32_t           mode;                       /*!<  File open mode (LIBSLAS_UPDATE, LIBSLAS_READONLY, LIBSLAS_READONLY_SEQUENTIAL).  */
  uint8_t           data;                       /*!<  Set if a point data record has been written to a new file (to test for VLR writing).  */
  int64_t           pos;                        /*!<  Position of the LAS file pointer after last I/O operation.  */
} INTERNAL_LIBSLAS_STRUCT;


/*  LIBSLAS error handling structure definition.  */

typedef struct 
{
  int32_t           libslas;                    /*!<  Last LIBSLAS error condition encountered.  */
  char              info[2048];                 /*!<  Text information to be printed out.  */
} LIBSLAS_ERROR_STRUCT;


/*!  This is where we'll store the headers and formatting/usage information of all open LIBSLAS files.  */

static INTERNAL_LIBSLAS_STRUCT las[LIBSLAS_MAX_FILES];


/*!  This is where we'll store error information in the event of some kind of screwup.  */

static LIBSLAS_ERROR_STRUCT libslas_error;


/*!  Startup flag used by either libslas_create_las_file or libslas_open_las_file to initialize the internal struct arrays and
     set the SIGINT handler.  */

static uint8_t first = 1;


/*  Include some public domain functions that we need here.  */

#include "libslas_pd_functions.h"


/********************************************************************************************/
/*!

 - Function:    libslas_swap_las_header

 - Purpose:     Byte swaps the LAS header on big endian systems.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        07/16/14

 - Arguments:
                - hnd            =    The LIBSLAS file handle

 - Returns:
                - void

 - Caveats:     This function is static, it is only used internal to the API and is not
                callable from an external program.

*********************************************************************************************/

static void libslas_swap_las_header (int32_t hnd)
{
  int16_t i;

  libslas_swap_uint16_t (&las[hnd].header.global_encoding);
  libslas_swap_uint32_t (&las[hnd].header.GUID_data_1);
  libslas_swap_uint16_t (&las[hnd].header.GUID_data_2);
  libslas_swap_uint16_t (&las[hnd].header.GUID_data_3);
  libslas_swap_uint16_t (&las[hnd].header.file_creation_DOY);
  libslas_swap_uint16_t (&las[hnd].header.file_creation_year);
  libslas_swap_uint16_t (&las[hnd].header.header_size);
  libslas_swap_uint32_t (&las[hnd].header.offset_to_point_data);
  libslas_swap_uint32_t (&las[hnd].header.number_of_VLRs);
  libslas_swap_uint16_t (&las[hnd].header.point_data_record_length);
  libslas_swap_uint32_t (&las[hnd].header.number_of_point_records);

  for (i = 0 ; i < 5 ; i++) libslas_swap_uint32_t (&las[hnd].header.number_of_points_by_return[i]);

  libslas_swap_double (&las[hnd].header.x_scale_factor);
  libslas_swap_double (&las[hnd].header.y_scale_factor);
  libslas_swap_double (&las[hnd].header.z_scale_factor);
  libslas_swap_double (&las[hnd].header.x_offset);
  libslas_swap_double (&las[hnd].header.y_offset);
  libslas_swap_double (&las[hnd].header.z_offset);
  libslas_swap_double (&las[hnd].header.max_x);
  libslas_swap_double (&las[hnd].header.min_x);
  libslas_swap_double (&las[hnd].header.max_y);
  libslas_swap_double (&las[hnd].header.min_y);
  libslas_swap_double (&las[hnd].header.max_z);
  libslas_swap_double (&las[hnd].header.min_z);
}



/********************************************************************************************/
/*!

 - Function:    libslas_write_header

 - Purpose:     Write the header to the LAS file.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        07/16/14

 - Arguments:
                - hnd            =    The LIBSLAS file handle

 - Returns:
                - LIBSLAS_SUCCESS
                - LIBSLAS_HEADER_WRITE_FSEEK_ERROR
                - LIBSLAS_HEADER_WRITE_ERROR

 - Caveats:     All returned error values are less than zero.  Success or a file handle
                will be greater than or equal to zero.  A simple test for failure is to
                check to see if the return is less than zero.

                This function is static, it is only used internal to the API and is not
                callable from an external program.

*********************************************************************************************/

static int32_t libslas_write_header (int32_t hnd)
{
  int16_t   i, pos;
  int32_t   hdr_size;
  uint8_t   header_data[LIBSLAS_HEADER_SIZE];


#ifdef LIBSLAS_DEBUG
  fprintf (LIBSLAS_DEBUG_OUTPUT, "%s %s %d Handle = %d\n", __FILE__, __FUNCTION__, __LINE__, hnd);
  fflush (LIBSLAS_DEBUG_OUTPUT);
#endif


  /*  Position to the beginning of the file.  */

  if (fseeko64 (las[hnd].fp, 0LL, SEEK_SET) < 0)
    {
      sprintf (libslas_error.info, _("File : %s\nError during fseek prior to writing LAS header :\n%s\nFunction: %s, Line: %d\n"), las[hnd].path, strerror (errno),
               __FUNCTION__, __LINE__ - 3);
      return (libslas_error.libslas = LIBSLAS_HEADER_WRITE_FSEEK_ERROR);
    }


  /*  Save the header size bfore we swap it (maybe).  */

  hdr_size = las[hnd].header.header_size;


  /*  Swap the fields in the header that need to be swapped on a big endian system.  */

  if (las[hnd].swap) libslas_swap_las_header (hnd);


  /*  Fill the header buffer with data.  */

  pos = 0;
  sprintf ((char *) &header_data[pos], "LASF"); pos += 4;
  memcpy (&header_data[pos], &las[hnd].header.file_source_id, 2); pos += 2;
  memcpy (&header_data[pos], &las[hnd].header.global_encoding, 2); pos += 2;
  memcpy (&header_data[pos], &las[hnd].header.GUID_data_1, 4); pos += 4;
  memcpy (&header_data[pos], &las[hnd].header.GUID_data_2, 2); pos += 2;
  memcpy (&header_data[pos], &las[hnd].header.GUID_data_3, 2); pos += 2;
  memcpy (&header_data[pos], &las[hnd].header.GUID_data_4, 8); pos += 8;
  memcpy (&header_data[pos], &las[hnd].header.version_major, 1); pos += 1;
  memcpy (&header_data[pos], &las[hnd].header.version_minor, 1); pos += 1;
  memcpy (&header_data[pos], &las[hnd].header.system_id, 32); pos += 32;
  memcpy (&header_data[pos], &las[hnd].header.generating_software, 32); pos += 32;
  memcpy (&header_data[pos], &las[hnd].header.file_creation_DOY, 2); pos += 2;
  memcpy (&header_data[pos], &las[hnd].header.file_creation_year, 2); pos += 2;
  memcpy (&header_data[pos], &las[hnd].header.header_size, 2); pos += 2;
  memcpy (&header_data[pos], &las[hnd].header.offset_to_point_data, 4); pos += 4;
  memcpy (&header_data[pos], &las[hnd].header.number_of_VLRs, 4); pos += 4;
  memcpy (&header_data[pos], &las[hnd].header.point_data_format_id, 1); pos += 1;
  memcpy (&header_data[pos], &las[hnd].header.point_data_record_length, 2); pos += 2;
  memcpy (&header_data[pos], &las[hnd].header.number_of_point_records, 4); pos += 4;
  for (i = 0 ; i < 5 ; i++)
    {
      memcpy (&header_data[pos], &las[hnd].header.number_of_points_by_return[i], 4);
      pos += 4;
    }
  memcpy (&header_data[pos], &las[hnd].header.x_scale_factor, 8); pos += 8;
  memcpy (&header_data[pos], &las[hnd].header.y_scale_factor, 8); pos += 8;
  memcpy (&header_data[pos], &las[hnd].header.z_scale_factor, 8); pos += 8;
  memcpy (&header_data[pos], &las[hnd].header.x_offset, 8); pos += 8;
  memcpy (&header_data[pos], &las[hnd].header.y_offset, 8); pos += 8;
  memcpy (&header_data[pos], &las[hnd].header.z_offset, 8); pos += 8;
  memcpy (&header_data[pos], &las[hnd].header.max_x, 8); pos += 8;
  memcpy (&header_data[pos], &las[hnd].header.min_x, 8); pos += 8;
  memcpy (&header_data[pos], &las[hnd].header.max_y, 8); pos += 8;
  memcpy (&header_data[pos], &las[hnd].header.min_y, 8); pos += 8;
  memcpy (&header_data[pos], &las[hnd].header.max_z, 8); pos += 8;
  memcpy (&header_data[pos], &las[hnd].header.min_z, 8); pos += 8;


  if (!fwrite (header_data, hdr_size, 1, las[hnd].fp))
    {
      sprintf (libslas_error.info, _("File : %s\nError writing LAS header :\n%s\nFunction: %s, Line: %d\n"), las[hnd].path, strerror (errno),
               __FUNCTION__, __LINE__ - 3);
      return (libslas_error.libslas = LIBSLAS_HEADER_WRITE_ERROR);
    }


  las[hnd].write = 1;
  las[hnd].pos = ftello64 (las[hnd].fp);


#ifdef LIBSLAS_DEBUG
  fprintf (LIBSLAS_DEBUG_OUTPUT, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
  fflush (LIBSLAS_DEBUG_OUTPUT);
#endif


  return (libslas_error.libslas = LIBSLAS_SUCCESS);
}



/********************************************************************************************/
/*!

 - Function:    libslas_read_header

 - Purpose:     Read the header from the LAS file.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        07/16/14

 - Arguments:
                - hnd            =    The LIBSLAS file handle

 - Returns:
                - LIBSLAS_SUCCESS
                - LIBSLAS_HEADER_READ_FSEEK_ERROR
                - LIBSLAS_HEADER_READ_ERROR
                - LIBSLAS_NOT_LAS_FILE_ERROR

 - Caveats:     All returned error values are less than zero.  Success or a file handle
                will be greater than or equal to zero.  A simple test for failure is to
                check to see if the return is less than zero.

                This function is static, it is only used internal to the API and is not
                callable from an external program.

*********************************************************************************************/

static int32_t libslas_read_header (int32_t hnd)
{
  int16_t   i, pos;
  uint8_t   header_data[LIBSLAS_HEADER_SIZE];


#ifdef LIBSLAS_DEBUG
  fprintf (LIBSLAS_DEBUG_OUTPUT, "%s %s %d Handle = %d\n", __FILE__, __FUNCTION__, __LINE__, hnd);
  fflush (LIBSLAS_DEBUG_OUTPUT);
#endif


  /*  Zero fill the header_data buffer.  */

  memset (header_data, 0, LIBSLAS_HEADER_SIZE);


  /*  Position to the beginning of the file.  */

  if (fseeko64 (las[hnd].fp, 0LL, SEEK_SET) < 0)
    {
      sprintf (libslas_error.info, _("File : %s\nError during fseek prior to reading LAS header :\n%s\nFunction: %s, Line: %d\n"), las[hnd].path, strerror (errno),
               __FUNCTION__, __LINE__ - 3);
      return (libslas_error.libslas = LIBSLAS_HEADER_READ_FSEEK_ERROR);
    }


  if (!fread (header_data, LIBSLAS_HEADER_SIZE, 1, las[hnd].fp))
    {
      sprintf (libslas_error.info, _("File : %s\nError reading LAS header :\n%s\nFunction: %s, Line: %d\n"), las[hnd].path, strerror (errno),
               __FUNCTION__, __LINE__ - 3);
      return (libslas_error.libslas = LIBSLAS_HEADER_READ_ERROR);
    }


  /*  Check the file signature.  */

  if (strncmp ((char *) header_data, "LASF", 4))
    {
      sprintf (libslas_error.info, _("File : %s\nThis is not a LAS file.\nFunction: %s, Line: %d\n"), las[hnd].path,
               __FUNCTION__, __LINE__ - 3);
      return (libslas_error.libslas = LIBSLAS_NOT_LAS_FILE_ERROR);
    }


  /*  Fill the header buffer with data.  */

  pos = 4;
  memcpy (&las[hnd].header.file_source_id, &header_data[pos], 2); pos += 2;
  memcpy (&las[hnd].header.global_encoding, &header_data[pos], 2); pos += 2;
  memcpy (&las[hnd].header.GUID_data_1, &header_data[pos], 4); pos += 4;
  memcpy (&las[hnd].header.GUID_data_2, &header_data[pos], 2); pos += 2;
  memcpy (&las[hnd].header.GUID_data_3, &header_data[pos], 2); pos += 2;
  memcpy (&las[hnd].header.GUID_data_4, &header_data[pos], 8); pos += 8;
  memcpy (&las[hnd].header.version_major, &header_data[pos], 1); pos += 1;
  memcpy (&las[hnd].header.version_minor, &header_data[pos], 1); pos += 1;
  memcpy (&las[hnd].header.system_id, &header_data[pos], 32); pos += 32;
  memcpy (&las[hnd].header.generating_software, &header_data[pos], 32); pos += 32;
  memcpy (&las[hnd].header.file_creation_DOY, &header_data[pos], 2); pos += 2;
  memcpy (&las[hnd].header.file_creation_year, &header_data[pos], 2); pos += 2;
  memcpy (&las[hnd].header.header_size, &header_data[pos], 2); pos += 2;
  memcpy (&las[hnd].header.offset_to_point_data, &header_data[pos], 4); pos += 4;
  memcpy (&las[hnd].header.number_of_VLRs, &header_data[pos], 4); pos += 4;
  memcpy (&las[hnd].header.point_data_format_id, &header_data[pos], 1); pos += 1;
  memcpy (&las[hnd].header.point_data_record_length, &header_data[pos], 2); pos += 2;
  memcpy (&las[hnd].header.number_of_point_records, &header_data[pos], 4); pos += 4;
  for (i = 0 ; i < 5 ; i++)
    {
      memcpy (&las[hnd].header.number_of_points_by_return[i], &header_data[pos], 4);
      pos += 4;
    }
  memcpy (&las[hnd].header.x_scale_factor, &header_data[pos], 8); pos += 8;
  memcpy (&las[hnd].header.y_scale_factor, &header_data[pos], 8); pos += 8;
  memcpy (&las[hnd].header.z_scale_factor, &header_data[pos], 8); pos += 8;
  memcpy (&las[hnd].header.x_offset, &header_data[pos], 8); pos += 8;
  memcpy (&las[hnd].header.y_offset, &header_data[pos], 8); pos += 8;
  memcpy (&las[hnd].header.z_offset, &header_data[pos], 8); pos += 8;
  memcpy (&las[hnd].header.max_x, &header_data[pos], 8); pos += 8;
  memcpy (&las[hnd].header.min_x, &header_data[pos], 8); pos += 8;
  memcpy (&las[hnd].header.max_y, &header_data[pos], 8); pos += 8;
  memcpy (&las[hnd].header.min_y, &header_data[pos], 8); pos += 8;
  memcpy (&las[hnd].header.max_z, &header_data[pos], 8); pos += 8;
  memcpy (&las[hnd].header.min_z, &header_data[pos], 8); pos += 8;


  /*  Swap the fields in the header that need to be swapped on a big endian system.  */

  if (las[hnd].swap) libslas_swap_las_header (hnd);


  /*  Check for v1.3 or greater.  */

  if (las[hnd].header.version_major == 1 && las[hnd].header.version_minor > 2)
    {
      sprintf (libslas_error.info, _("File : %s\nSorry, libslas doesn't support version 1.3 or newer LAS files.\nFunction: %s, Line: %d\n"), las[hnd].path,
               __FUNCTION__, __LINE__ - 3);
      return (libslas_error.libslas = LIBSLAS_INCORRECT_VERSION_ERROR);
    }


  las[hnd].write = 0;
  las[hnd].pos = ftello64 (las[hnd].fp);


#ifdef LIBSLAS_DEBUG
  fprintf (LIBSLAS_DEBUG_OUTPUT, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
  fflush (LIBSLAS_DEBUG_OUTPUT);
#endif


  return (libslas_error.libslas = LIBSLAS_SUCCESS);
}



/********************************************************************************************/
/*!

 - Function:    libslas_create_las_file

 - Purpose:     Create a LAS file.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        07/16/14

 - Arguments:
                - path           =    Path name
                - header         =    LIBSLAS_HEADER structure to be written to the file

 - Returns:     
                - The file handle (0 or positive)
                - LIBSLAS_TOO_MANY_OPEN_FILES_ERROR
                - LIBSLAS_LAS_CREATE_ERROR
                - Error value from libslas_write_header

 - Caveats:     All returned error values are less than zero.  Success or a file handle
                will be greater than or equal to zero.  A simple test for failure is to
                check to see if the return is less than zero.

                Please look at the LIBSLAS_HEADER structure in the libslas.h file to determine which
                fields must be set by your application in the header structure prior to creating
                the LAS file.

*********************************************************************************************/

int32_t libslas_create_las_file (char *path, LIBSLAS_HEADER *header)
{
  int32_t i, hnd;


#ifdef LIBSLAS_DEBUG
  fprintf (LIBSLAS_DEBUG_OUTPUT, "%s %s %d Path = %s\n", __FILE__, __FUNCTION__, __LINE__, path);
  fflush (LIBSLAS_DEBUG_OUTPUT);
#endif


  /*  The first time through we want to initialize all of the LIBSLAS file pointers.  */

  if (first)
    {
      for (i = 0 ; i < LIBSLAS_MAX_FILES ; i++) las[i].fp = NULL;


      /*  Set up the SIGINT handler.  */

      signal (SIGINT, libslas_sigint_handler);

      first = 0;
    }


  /*  Find the next available handle and make sure we haven't opened too many.  Also, zero the internal record structure.  */

  hnd = LIBSLAS_MAX_FILES;
  for (i = 0 ; i < LIBSLAS_MAX_FILES ; i++)
    {
      if (las[i].fp == NULL)
        {
          memset (&las[i], 0, sizeof (INTERNAL_LIBSLAS_STRUCT));

          hnd = i;
          break;
        }
    }


  if (hnd == LIBSLAS_MAX_FILES)
    {
      sprintf (libslas_error.info, _("Too many LIBSLAS files are already open.\nFunction: %s, Line: %d\n"),
               __FUNCTION__, __LINE__ - 3);
      return (libslas_error.libslas = LIBSLAS_TOO_MANY_OPEN_FILES_ERROR);
    }


  /*  Save the file name for error messages.  */

  strcpy (las[hnd].path, path);


  /*  Make sure that the file has a .las extension.  */

  if (strcmp (&path[strlen (path) - 4], ".las"))
    {
      sprintf (libslas_error.info, _("File : %s\nInvalid file extension for LAS file (must be .las)\nFunction: %s, Line: %d\n"), las[hnd].path,
               __FUNCTION__, __LINE__ - 3);
      return (libslas_error.libslas = LIBSLAS_INVALID_FILENAME_ERROR);
    }


  /*  Set the standard fields.  */

  header->version_major = 1;
  header->version_minor = 2;
  header->header_size = LIBSLAS_HEADER_SIZE;
  header->offset_to_point_data = 0;
  header->number_of_point_records = 0;
  header->max_x = header->max_y = header->max_z = -99999999999999.0;
  header->min_x = header->min_y = header->min_z = 99999999999999.0;

  switch (header->point_data_format_id)
    {
    case 0:
      header->point_data_record_length = 20;
      break;

    case 1:
      header->point_data_record_length = 28;
      break;

    case 2:
      header->point_data_record_length = 26;
      break;

    case 3:
      header->point_data_record_length = 34;
      break;

    default:
      sprintf (libslas_error.info, _("File : %s\nInvalid point format id (%d) specified for LAS file (must be .las)\nFunction: %s, Line: %d\n"), las[hnd].path,
               header->point_data_format_id, __FUNCTION__, __LINE__ - 3);
      return (libslas_error.libslas = LIBSLAS_INVALID_POINT_FORMAT_ID_ERROR);
    }

  for (i = 0 ; i < 5 ; i++) header->number_of_points_by_return[i] = 0;


  /*  Check global encoding value.  */

  if (header->global_encoding < 0 || header->global_encoding > 1)
    {
      sprintf (libslas_error.info, _("File : %s\nInvalid global encoding value (%d) specified for LAS file (must be 0 or 1)\nFunction: %s, Line: %d\n"), las[hnd].path, 
               header->point_data_format_id, __FUNCTION__, __LINE__ - 3);
      return (libslas_error.libslas = LIBSLAS_INVALID_GLOBAL_ENCODING_ERROR);
    }


  /*  Save the entire header to the LIBSLAS internal data structure.  */

  las[hnd].header = *header;


  /*  Open the file.  */

  if ((las[hnd].fp = fopen64 (las[hnd].path, "wb+")) == NULL)
    {
      sprintf (libslas_error.info, _("File : %s\nError creating LAS file :\n%s\nFunction: %s, Line: %d\n"), las[hnd].path, strerror (errno),
               __FUNCTION__, __LINE__ - 3);
      return (libslas_error.libslas = LIBSLAS_LAS_CREATE_ERROR);
    }


  /*  Write the LAS header.  */

  if (libslas_write_header (hnd) < 0)
    {
      fclose (las[hnd].fp);
      las[hnd].fp = NULL;

      return (libslas_error.libslas);
    }

  las[hnd].at_end = 1;
  las[hnd].modified = 1;
  las[hnd].created = 1;
  las[hnd].write = 1;


#ifdef LIBSLAS_DEBUG
  fprintf (LIBSLAS_DEBUG_OUTPUT, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
  fflush (LIBSLAS_DEBUG_OUTPUT);
#endif


  return (hnd);
}



/********************************************************************************************/
/*!

 - Function:    libslas_open_las_file

 - Purpose:     Open a LAS file.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        07/17/14

 - Arguments:
                - path           =    Path name
                - header         =    LIBSLAS_HEADER structure to be populated
                - mode           =    LIBSLAS_UPDATE or LIBSLAS_READONLY

 - Returns:
                - The file handle (0 or positive)
                - Error value from libslas_open_las_file

*********************************************************************************************/

int32_t libslas_open_las_file (char *path, LIBSLAS_HEADER *header, int32_t mode)
{
  int32_t i, hnd;


#ifdef LIBSLAS_DEBUG
  fprintf (LIBSLAS_DEBUG_OUTPUT, "%s %s %d Path = %s\n", __FILE__, __FUNCTION__, __LINE__, path);
  fflush (LIBSLAS_DEBUG_OUTPUT);
#endif


  /*  The first time through we want to initialize all of the LIBSLAS file pointers.  */

  if (first)
    {
      for (i = 0 ; i < LIBSLAS_MAX_FILES ; i++) las[i].fp = NULL;


      /*  Set up the SIGINT handler.  */

      signal (SIGINT, libslas_sigint_handler);

      first = 0;
    }


  /*  Find the next available handle and make sure we haven't opened too many.  Also, zero the internal record structure.  */

  hnd = LIBSLAS_MAX_FILES;
  for (i = 0 ; i < LIBSLAS_MAX_FILES ; i++)
    {
      if (las[i].fp == NULL)
        {
          memset (&las[i], 0, sizeof (INTERNAL_LIBSLAS_STRUCT));

          hnd = i;
          break;
        }
    }


  if (hnd == LIBSLAS_MAX_FILES)
    {
      sprintf (libslas_error.info, _("Too many LIBSLAS files are already open.\nFunction: %s, Line: %d\n"),
               __FUNCTION__, __LINE__ - 3);
      return (libslas_error.libslas = LIBSLAS_TOO_MANY_OPEN_FILES_ERROR);
    }


  /*  Save the file name for error messages.  */

  strcpy (las[hnd].path, path);


  /*  Make sure that the file has a .las extension.  */

  if (strcmp (&path[strlen (path) - 4], ".las"))
    {
      sprintf (libslas_error.info, _("File : %s\nInvalid file extension for LAS file (must be .las)\nFunction: %s, Line: %d\n"), las[hnd].path,
               __FUNCTION__, __LINE__ - 3);
      return (libslas_error.libslas = LIBSLAS_INVALID_FILENAME_ERROR);
    }


  if (libslas_big_endian ()) las[hnd].swap = 1;


  /*  Open the file and read the header.  */

  switch (mode)
    {
    case LIBSLAS_UPDATE:
      if ((las[hnd].fp = fopen64 (path, "rb+")) == NULL)
        {
          sprintf (libslas_error.info, _("File : %s\nError opening LAS file for update :\n%s\nFunction: %s, Line: %d\n"), las[hnd].path, strerror (errno),
               __FUNCTION__, __LINE__ - 3);
          return (libslas_error.libslas = LIBSLAS_OPEN_UPDATE_ERROR);
        }

      break;


    case LIBSLAS_READONLY:

      if ((las[hnd].fp = fopen64 (path, "rb")) == NULL)
        {
          sprintf (libslas_error.info, _("File : %s\nError opening LAS file read-only :\n%s\nFunction: %s, Line: %d\n"), las[hnd].path, strerror (errno),
               __FUNCTION__, __LINE__ - 3);
          return (libslas_error.libslas = LIBSLAS_OPEN_READONLY_ERROR);
        }

      break;

    default:
      sprintf (libslas_error.info, _("File : %s\nInvalid file open mode %d specified for file :\n%s\nFunction: %s, Line: %d\n"), las[hnd].path, mode, strerror (errno),
               __FUNCTION__, __LINE__ - 3);
      return (libslas_error.libslas = LIBSLAS_INVALID_MODE_ERROR);
    }


  /*  Save the open mode.  */

  las[hnd].mode = mode;


  /*  Read the header.  */

  if (libslas_read_header (hnd))
    {
      fclose (las[hnd].fp);
      las[hnd].fp = NULL;

      return (libslas_error.libslas);
    }


  *header = las[hnd].header;


  las[hnd].at_end = 0;
  las[hnd].modified = 0;
  las[hnd].created = 0;
  las[hnd].write = 0;


#ifdef LIBSLAS_DEBUG
  fprintf (LIBSLAS_DEBUG_OUTPUT, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
  fflush (LIBSLAS_DEBUG_OUTPUT);
#endif


  return (hnd);
}



/********************************************************************************************/
/*!

 - Function:    libslas_close_las_file

 - Purpose:     Close a LAS file.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        07/16/14

 - Arguments:
                - hnd            =    The file handle

 - Returns:
                - LIBSLAS_SUCCESS
                - Error value from libslas_write_header
                - LIBSLAS_CLOSE_ERROR

 - Caveats:     All returned error values are less than zero.  Success or a file handle
                will be greater than or equal to zero.  A simple test for failure is to
                check to see if the return is less than zero.

*********************************************************************************************/

int32_t libslas_close_las_file (int32_t hnd)
{
#ifdef LIBSLAS_DEBUG
  fprintf (LIBSLAS_DEBUG_OUTPUT, "%s %s %d Handle = %d\n", __FILE__, __FUNCTION__, __LINE__, hnd);
  fflush (LIBSLAS_DEBUG_OUTPUT);
#endif


  /*  Just in case someone tries to close a file more than once... */

  if (las[hnd].fp == NULL) return (libslas_error.libslas = LIBSLAS_SUCCESS);


  /*  If the LAS file was created we need to update the header.  */

  if (las[hnd].created)
    {
      if (libslas_write_header (hnd) < 0) return (libslas_error.libslas);
    }


  /*  Close the file.  */

  if (fclose (las[hnd].fp))
    {
      sprintf (libslas_error.info, _("File : %s\nError closing LAS file :\n%s\nFunction: %s, Line: %d\n"), las[hnd].path, strerror (errno),
               __FUNCTION__, __LINE__ - 3);
      return (libslas_error.libslas = LIBSLAS_CLOSE_ERROR);
    }


  /*  Clear the internal LIBSLAS structure.  */

  memset (&las[hnd], 0, sizeof (INTERNAL_LIBSLAS_STRUCT));


  /*  Set the file pointer to NULL so we can reuse the structure the next create/open.  */

  las[hnd].fp = NULL;


#ifdef LIBSLAS_DEBUG
  fprintf (LIBSLAS_DEBUG_OUTPUT, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
  fflush (LIBSLAS_DEBUG_OUTPUT);
#endif


  return (libslas_error.libslas = LIBSLAS_SUCCESS);
}



/*********************************************************************************************/
/*!

 - Function:    libslas_read_vlr_header

 - Purpose:     Retrieve a LIBSLAS VLR record.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        07/16/14

 - Arguments:
                - hnd            =    The file handle
                - recnum         =    The VLR record number (between 0 and number_of_VLRs from
                                      the LAS header)
                - vlr_header     =    The returned LIBSLAS VLR header

 - Returns:
                - LIBSLAS_SUCCESS
                - LIBSLAS_INVALID_VLR_RECORD_NUMBER_ERROR
                - LIBSLAS_VLR_HEADER_READ_FSEEK_ERROR
                - LIBSLAS_VLR_HEADER_READ_ERROR

 - Caveats:     This function ONLY returns the header.  I'm trying to avoid doing any memory
                allocation in the API.  Normally you would call this to get the header, allocate
                your own data buffer, then call libslas_read_vlr_data to get the contents of the
                VLR.  That way all memory allocation and de-allocation is on the application 
                side and not hidden away in the API.

                All returned error values are less than zero.  Success or a file handle
                will be greater than or equal to zero.  A simple test for failure is to
                check to see if the return is less than zero.

*********************************************************************************************/

int32_t libslas_read_vlr_header (int32_t hnd, int32_t recnum, LIBSLAS_VLR_HEADER *vlr_header)
{
  int32_t i;
  int64_t pos;


  /*  Check for record out of bounds.  */

  if (recnum >= las[hnd].header.number_of_VLRs || recnum < 0)
    {
      sprintf (libslas_error.info, _("File : %s\nVLR Record : %d\nInvalid VLR record number.\nFunction: %s, Line: %d\n"), las[hnd].path, recnum,
               __FUNCTION__, __LINE__ - 3);
      return (libslas_error.libslas = LIBSLAS_INVALID_VLR_RECORD_NUMBER_ERROR);
    }


  pos = (int64_t) las[hnd].header.header_size;


  if (fseeko64 (las[hnd].fp, pos, SEEK_SET) < 0)
    {
      sprintf (libslas_error.info, _("File : %s\nError during fseek prior to reading VLR header 0 :\n%s\nFunction: %s, Line: %d\n"), las[hnd].path, strerror (errno),
               __FUNCTION__, __LINE__ - 3);
      return (libslas_error.libslas = LIBSLAS_VLR_READ_FSEEK_ERROR);
    }


  for (i = 0 ; i < las[hnd].header.number_of_VLRs ; i++)
    {

      if (!fread (&vlr_header->reserved, 2, 1, las[hnd].fp))
        {
          sprintf (libslas_error.info, _("File : %s\nError reading VLR header (reserved) :\n%s\nFunction: %s, Line: %d\n"), las[hnd].path, strerror (errno),
               __FUNCTION__, __LINE__ - 3);
          return (libslas_error.libslas = LIBSLAS_VLR_READ_ERROR);
        }

      if (!fread (&vlr_header->user_id, 16, 1, las[hnd].fp))
        {
          sprintf (libslas_error.info, _("File : %s\nError reading VLR header (user id) :\n%s\nFunction: %s, Line: %d\n"), las[hnd].path, strerror (errno),
               __FUNCTION__, __LINE__ - 3);
          return (libslas_error.libslas = LIBSLAS_VLR_READ_ERROR);
        }

      if (!fread (&vlr_header->record_id, 2, 1, las[hnd].fp))
        {
          sprintf (libslas_error.info, _("File : %s\nError reading VLR header (record id) :\n%s\nFunction: %s, Line: %d\n"), las[hnd].path, strerror (errno),
               __FUNCTION__, __LINE__ - 3);
          return (libslas_error.libslas = LIBSLAS_VLR_READ_ERROR);
        }

      if (!fread (&vlr_header->record_length_after_header, 2, 1, las[hnd].fp))
        {
          sprintf (libslas_error.info, _("File : %s\nError reading VLR header (record length) :\n%s\nFunction: %s, Line: %d\n"), las[hnd].path, strerror (errno),
               __FUNCTION__, __LINE__ - 3);
          return (libslas_error.libslas = LIBSLAS_VLR_READ_ERROR);
        }

      if (!fread (&vlr_header->description, 32, 1, las[hnd].fp))
        {
          sprintf (libslas_error.info, _("File : %s\nError reading VLR header (description) :\n%s\nFunction: %s, Line: %d\n"), las[hnd].path, strerror (errno),
               __FUNCTION__, __LINE__ - 3);
          return (libslas_error.libslas = LIBSLAS_VLR_READ_ERROR);
        }


      /*  Swap things if we need to.  */

      if (las[hnd].swap)
        {
          libslas_swap_uint16_t (&vlr_header->record_id);
          libslas_swap_uint16_t (&vlr_header->record_length_after_header);
        }


      /*  If this is the one we want, return it to the caller.  */

      if (i == recnum) break;


      /*  Otherwise, move past the data block and get the next one.  */

      pos += vlr_header->record_length_after_header;


      if (fseeko64 (las[hnd].fp, pos, SEEK_SET) < 0)
        {
          sprintf (libslas_error.info, _("File : %s\nError during fseek prior to reading VLR header %d :\n%s\nFunction: %s, Line: %d\n"), las[hnd].path, i,
                   strerror (errno), __FUNCTION__, __LINE__ - 3);
          return (libslas_error.libslas = LIBSLAS_VLR_READ_FSEEK_ERROR);
        }
    }


  las[hnd].write = 0;
  las[hnd].at_end = 0;
  las[hnd].pos = ftello64 (las[hnd].fp);


  return (libslas_error.libslas = LIBSLAS_SUCCESS);
}



/*********************************************************************************************/
/*!

 - Function:    libslas_read_vlr_data

 - Purpose:     Reads the data for a VLR record.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        07/16/14

 - Arguments:
                - hnd            =    The file handle
                - recnum         =    The VLR record number (between 0 and number_of_VLRs from
                                      the LAS header)
                - vlr_data       =    The returned unsigned character data array.

 - Returns:
                - LIBSLAS_SUCCESS
                - LIBSLAS_INVALID_VLR_RECORD_NUMBER_ERROR
                - LIBSLAS_VLR_DATA_READ_FSEEK_ERROR
                - LIBSLAS_VLR_DATA_READ_ERROR

 - Caveats:     All returned error values are less than zero.  A simple test for failure is to
                check to see if the return is less than zero.

*********************************************************************************************/

int32_t libslas_read_vlr_data (int32_t hnd, int32_t recnum, uint8_t *vlr_data)
{
  int32_t             i;
  int64_t             pos;
  LIBSLAS_VLR_HEADER  hdr;
  uint16_t            tmp_short;
  double              tmp_double;


  /*  Check for record out of bounds.  */

  if (recnum >= las[hnd].header.number_of_VLRs || recnum < 0)
    {
      sprintf (libslas_error.info, _("File : %s\nVLR Record : %d\nInvalid VLR record number.\nFunction: %s, Line: %d\n"), las[hnd].path, recnum,
               __FUNCTION__, __LINE__ - 3);
      return (libslas_error.libslas = LIBSLAS_INVALID_VLR_RECORD_NUMBER_ERROR);
    }


  pos = (int64_t) las[hnd].header.header_size;

  if (fseeko64 (las[hnd].fp, pos, SEEK_SET) < 0)
    {
      sprintf (libslas_error.info, _("File : %s\nError during fseek prior to reading VLR data 0 :\n%s\nFunction: %s, Line: %d\n"), las[hnd].path, strerror (errno),
               __FUNCTION__, __LINE__ - 3);
      return (libslas_error.libslas = LIBSLAS_VLR_READ_FSEEK_ERROR);
    }


  for (i = 0 ; i < las[hnd].header.number_of_VLRs ; i++)
    {
      if (!fread (&hdr.reserved, 2, 1, las[hnd].fp))
        {
          sprintf (libslas_error.info, _("File : %s\nError reading VLR header (reserved) :\n%s\nFunction: %s, Line: %d\n"), las[hnd].path, strerror (errno),
               __FUNCTION__, __LINE__ - 3);
          return (libslas_error.libslas = LIBSLAS_VLR_READ_ERROR);
        }

      if (!fread (&hdr.user_id, 16, 1, las[hnd].fp))
        {
          sprintf (libslas_error.info, _("File : %s\nError reading VLR header (user id) :\n%s\nFunction: %s, Line: %d\n"), las[hnd].path, strerror (errno),
               __FUNCTION__, __LINE__ - 3);
          return (libslas_error.libslas = LIBSLAS_VLR_READ_ERROR);
        }

      if (!fread (&hdr.record_id, 2, 1, las[hnd].fp))
        {
          sprintf (libslas_error.info, _("File : %s\nError reading VLR header (record id) :\n%s\nFunction: %s, Line: %d\n"), las[hnd].path, strerror (errno),
               __FUNCTION__, __LINE__ - 3);
          return (libslas_error.libslas = LIBSLAS_VLR_READ_ERROR);
        }

      if (!fread (&hdr.record_length_after_header, 2, 1, las[hnd].fp))
        {
          sprintf (libslas_error.info, _("File : %s\nError reading VLR header (record length) :\n%s\nFunction: %s, Line: %d\n"), las[hnd].path, strerror (errno),
               __FUNCTION__, __LINE__ - 3);
          return (libslas_error.libslas = LIBSLAS_VLR_READ_ERROR);
        }

      if (!fread (&hdr.description, 32, 1, las[hnd].fp))
        {
          sprintf (libslas_error.info, _("File : %s\nError reading VLR header (description) :\n%s\nFunction: %s, Line: %d\n"), las[hnd].path, strerror (errno),
               __FUNCTION__, __LINE__ - 3);
          return (libslas_error.libslas = LIBSLAS_VLR_READ_ERROR);
        }


      /*  Swap things if we need to.  */

      if (las[hnd].swap)
        {
          libslas_swap_uint16_t (&hdr.record_id);
          libslas_swap_uint16_t (&hdr.record_length_after_header);
        }


      /*  If this is the one we want, read the data.  */

      if (i == recnum)
        {
          /*  First we read the data.  */

          if (!fread (vlr_data, hdr.record_length_after_header, 1, las[hnd].fp))
            {
              sprintf (libslas_error.info, _("File : %s\nError reading VLR data :\n%s\nFunction: %s, Line: %d\n"), las[hnd].path, strerror (errno),
               __FUNCTION__, __LINE__ - 3);
              return (libslas_error.libslas = LIBSLAS_VLR_READ_ERROR);
            }


          /*  Swap things if we need to.  */

          if (las[hnd].swap)
            {
              switch (hdr.record_id)
                {
                case 34735:
                  for (i = 0 ; i < hdr.record_length_after_header ; i += 2)
                    {
                      memcpy (&vlr_data[i], &tmp_short, 2);
                      libslas_swap_uint16_t (&tmp_short);
                      memcpy (&tmp_short, &vlr_data[i], 2);
                    }
                  break;

                case 34736:
                  for (i = 0 ; i < hdr.record_length_after_header ; i += 8)
                    {
                      memcpy (&vlr_data[i], &tmp_double, 8);
                      libslas_swap_double (&tmp_double);
                      memcpy (&tmp_double, &vlr_data[i], 8);
                    }
                  break;
                }
            }

          break;
        }
      else
        {
          /*  Otherwise, move past the data block and get the next one.  */

          pos += hdr.record_length_after_header;


          if (fseeko64 (las[hnd].fp, pos, SEEK_SET) < 0)
            {
              sprintf (libslas_error.info, _("File : %s\nError during fseek prior to reading VLR header %d :\n%s\nFunction: %s, Line: %d\n"), las[hnd].path, i,
                       strerror (errno), __FUNCTION__, __LINE__ - 3);
              return (libslas_error.libslas = LIBSLAS_VLR_READ_FSEEK_ERROR);
            }
        }
    }


  las[hnd].write = 0;
  las[hnd].at_end = 0;
  las[hnd].pos = ftello64 (las[hnd].fp);


  return (libslas_error.libslas = LIBSLAS_SUCCESS);
}



/********************************************************************************************/
/*!

 - Function:    libslas_append_vlr_record

 - Purpose:     Appends a LAS Variable length record to the LAS file (after the header).

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        07/16/14

 - Arguments:
                - hnd            =    The file handle
                - vlr_header     =    The LIBSLAS_VLR_HEADER structure (see libslas.h)
                - vlr_data       =    An unsigned character array containing the data for the VLR.

 - Returns:
                - LIBSLAS_SUCCESS
                - LIBSLAS_VLR_APPEND_ERROR
                - LIBSLAS_VLR_WRITE_FSEEK_ERROR
                - LIBSLAS_VLR_WRITE_ERROR

 - Caveats:     This function is ONLY used to append a new vlr record to a file as it is being
                created.  VLR records must be written prior to and point data records so that
                the API can keep track of the needed data offset value.

                All returned error values are less than zero.  Success or a file handle
                will be greater than or equal to zero.  A simple test for failure is to
                check to see if the return is less than zero.

*********************************************************************************************/

int32_t libslas_append_vlr_record (int32_t hnd, LIBSLAS_VLR_HEADER *vlr_header, uint8_t *vlr_data)
{
  int32_t   i, rec_length;
  uint16_t  tmp_short;
  double    tmp_double;


  /*  Appending a VLR is only allowed if you are creating a new file and you haven't written any point records.  */

  if (!las[hnd].created)
    {
      sprintf (libslas_error.info, _("File : %s\nAppending VLR records pre-existing LAS file not allowed.\nFunction: %s, Line: %d\n"), las[hnd].path,
               __FUNCTION__, __LINE__ - 3);
      return (libslas_error.libslas = LIBSLAS_VLR_APPEND_ERROR);
    }


  if (las[hnd].data)
    {
      sprintf (libslas_error.info, _("File : %s\nVLR records can not be added after writing any point data records.\nFunction: %s, Line: %d\n"), las[hnd].path,
               __FUNCTION__, __LINE__ - 3);
      return (libslas_error.libslas = LIBSLAS_VLR_APPEND_ERROR);
    }


  /*  If we're not already at the end of the file...  */

  if (!las[hnd].at_end)
    {
      /*  We're appending so we need to seek to the end of the file.  */

      if (fseeko64 (las[hnd].fp, 0, SEEK_END) < 0)
        {
          sprintf (libslas_error.info, _("File : %s\nError during fseek prior to writing VLR record :\n%s\nFunction: %s, Line: %d\n"), las[hnd].path, strerror (errno),
               __FUNCTION__, __LINE__ - 3);
          return (libslas_error.libslas = LIBSLAS_VLR_WRITE_FSEEK_ERROR);
        }
    }


  las[hnd].at_end = 1;


  /*  Set reserved field to 0.  */

  vlr_header->reserved = 0;


  rec_length = vlr_header->record_length_after_header;


  /*  Swap things if we need to.  */

  if (las[hnd].swap)
    {
      switch (vlr_header->record_id)
        {
        case 34735:
          for (i = 0 ; i < vlr_header->record_length_after_header ; i += 2)
            {
              memcpy (&vlr_data[i], &tmp_short, 2);
              libslas_swap_uint16_t (&tmp_short);
              memcpy (&tmp_short, &vlr_data[i], 2);
            }
          break;

        case 34736:
          for (i = 0 ; i < vlr_header->record_length_after_header ; i += 8)
            {
              memcpy (&vlr_data[i], &tmp_double, 8);
              libslas_swap_double (&tmp_double);
              memcpy (&tmp_double, &vlr_data[i], 8);
            }
          break;
        }

      libslas_swap_uint16_t (&vlr_header->record_id);
      libslas_swap_uint16_t (&vlr_header->record_length_after_header);
    }


  if (!fwrite (&vlr_header->reserved, 2, 1, las[hnd].fp))
    {
      sprintf (libslas_error.info, _("File : %s\nError writing VLR header (reserved) :\n%s\nFunction: %s, Line: %d\n"), las[hnd].path, strerror (errno),
               __FUNCTION__, __LINE__ - 3);
      return (libslas_error.libslas = LIBSLAS_VLR_WRITE_ERROR);
    }

  if (!fwrite (&vlr_header->user_id, 16, 1, las[hnd].fp))
    {
      sprintf (libslas_error.info, _("File : %s\nError writing VLR header (user id) :\n%s\nFunction: %s, Line: %d\n"), las[hnd].path, strerror (errno),
               __FUNCTION__, __LINE__ - 3);
      return (libslas_error.libslas = LIBSLAS_VLR_WRITE_ERROR);
    }

  if (!fwrite (&vlr_header->record_id, 2, 1, las[hnd].fp))
    {
      sprintf (libslas_error.info, _("File : %s\nError writing VLR header (record id) :\n%s\nFunction: %s, Line: %d\n"), las[hnd].path, strerror (errno),
               __FUNCTION__, __LINE__ - 3);
      return (libslas_error.libslas = LIBSLAS_VLR_WRITE_ERROR);
    }

  if (!fwrite (&vlr_header->record_length_after_header, 2, 1, las[hnd].fp))
    {
      sprintf (libslas_error.info, _("File : %s\nError writing VLR header (record length) :\n%s\nFunction: %s, Line: %d\n"), las[hnd].path, strerror (errno),
               __FUNCTION__, __LINE__ - 3);
      return (libslas_error.libslas = LIBSLAS_VLR_WRITE_ERROR);
    }

  if (!fwrite (&vlr_header->description, 32, 1, las[hnd].fp))
    {
      sprintf (libslas_error.info, _("File : %s\nError writing VLR header (description) :\n%s\nFunction: %s, Line: %d\n"), las[hnd].path, strerror (errno),
               __FUNCTION__, __LINE__ - 3);
      return (libslas_error.libslas = LIBSLAS_VLR_WRITE_ERROR);
    }

  if (!fwrite (vlr_data, rec_length, 1, las[hnd].fp))
    {
      sprintf (libslas_error.info, _("File : %s\nError writing VLR data :\n%s\nFunction: %s, Line: %d\n"), las[hnd].path, strerror (errno),
               __FUNCTION__, __LINE__ - 3);
      return (libslas_error.libslas = LIBSLAS_VLR_WRITE_ERROR);
    }


  /*  Set the offset to the point data to be just after the last VLR written.  */

  las[hnd].pos = las[hnd].header.offset_to_point_data = ftello64 (las[hnd].fp);

  las[hnd].at_end = 1;
  las[hnd].write = 1;


#ifdef LIBSLAS_DEBUG
  fprintf (LIBSLAS_DEBUG_OUTPUT, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
  fflush (LIBSLAS_DEBUG_OUTPUT);
#endif


  return (libslas_error.libslas = LIBSLAS_SUCCESS);
}



/********************************************************************************************/
/*!

 - Function:    libslas_read_point_data

 - Purpose:     Retrieve a LAS point data record.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        07/16/14

 - Arguments:
                - hnd            =    The file handle
                - recnum         =    The record number of the LAS point data record to be
                                      retrieved (records start at 0)
                - record         =    The returned LAS point data record

 - Returns:
                - LIBSLAS_SUCCESS
                - LIBSLAS_INVALID_RECORD_NUMBER_ERROR
                - LIBSLAS_READ_FSEEK_ERROR
                - LIBSLAS_READ_ERROR

 - Caveats:     All returned error values are less than zero.  Success or a file handle
                will be greater than or equal to zero.  A simple test for failure is to
                check to see if the return is less than zero.

                Note that we only have one point data format structure.  Since each of the 
                point data formats was a subset of POINT DATA RECORD FORMAT 3 it's much easier
                to just deal with one format.  If you happen to read a POINT DATA RECORD
                FORMAT 0 record then gps_time, red, green, and blue will be set to 0.  These
                records are so tiny that I'm not worried about the wasted space in memory.
                If you read a billion FORMAT 0 records into memory (which would be a very
                stupid thing to do) you would only be wasting 14GB.

*********************************************************************************************/

int32_t libslas_read_point_data (int32_t hnd, int32_t recnum, LIBSLAS_POINT_DATA *record)
{
  int32_t  x, y, z, pos;
  int64_t  addr;
  uint8_t  data[POINT_DATA_SIZE], rets, cls;


  /*  Check for record out of bounds.  */

  if (recnum >= las[hnd].header.number_of_point_records || recnum < 0)
    {
      sprintf (libslas_error.info, _("File : %s\nRecord : %d\nInvalid record number.\nFunction: %s, Line: %d\n"), las[hnd].path, recnum,
               __FUNCTION__, __LINE__ - 3);
      return (libslas_error.libslas = LIBSLAS_INVALID_RECORD_NUMBER_ERROR);
    }


  addr = (int64_t) las[hnd].header.offset_to_point_data + (int64_t) las[hnd].header.point_data_record_length * (int64_t) recnum;


  /*  Don't do the fseek if we're already at the correct point.  */

  if (las[hnd].pos != addr)
    {
      if (fseeko64 (las[hnd].fp, addr, SEEK_SET) < 0)
        {
          sprintf (libslas_error.info, _("File : %s\nError during fseek prior to reading LAS record :\n%s\nFunction: %s, Line: %d\n"), las[hnd].path, strerror (errno),
               __FUNCTION__, __LINE__ - 3);
          return (libslas_error.libslas = LIBSLAS_READ_FSEEK_ERROR);
        }
    }


  memset (record, 0, sizeof (LIBSLAS_POINT_DATA));
  memset (data, 0, POINT_DATA_SIZE);


  /*  Read the record.  */

  if (!fread (data, las[hnd].header.point_data_record_length, 1, las[hnd].fp))
    {
      sprintf (libslas_error.info, _("File : %s\nRecord : %d\nError reading LAS record :\n%s\nFunction: %s, Line: %d\n"), las[hnd].path, recnum, strerror (errno),
               __FUNCTION__, __LINE__ - 3);
      return (libslas_error.libslas = LIBSLAS_READ_ERROR);
    }


  /*  Set the current position.  */

  las[hnd].pos = ftello64 (las[hnd].fp);


  /*  Get the data out of the buffer.  */

  pos = 0;
  memcpy (&x, &data[pos], 4); pos += 4;
  memcpy (&y, &data[pos], 4); pos += 4;
  memcpy (&z, &data[pos], 4); pos += 4;
  memcpy (&record->intensity, &data[pos], 2); pos += 2;
  memcpy (&rets, &data[pos], 1); pos += 1;
  memcpy (&cls, &data[pos], 1); pos += 1;
  memcpy (&record->scan_angle_rank, &data[pos], 1); pos += 1;
  memcpy (&record->user_data, &data[pos], 1); pos += 1;
  memcpy (&record->point_source_id, &data[pos], 2); pos += 2;


  switch (las[hnd].header.point_data_format_id)
    {
    case 1:
      memcpy (&record->gps_time, &data[pos], 8); pos += 8;
      break;

    case 2:
      memcpy (&record->red, &data[pos], 2); pos += 2;
      memcpy (&record->green, &data[pos], 2); pos += 2;
      memcpy (&record->blue, &data[pos], 2); pos += 2;
      break;

    case 3:
      memcpy (&record->gps_time, &data[pos], 8); pos += 8;
      memcpy (&record->red, &data[pos], 2); pos += 2;
      memcpy (&record->green, &data[pos], 2); pos += 2;
      memcpy (&record->blue, &data[pos], 2); pos += 2;
      break;
    }


  /*  If we have to swap the record, do so.  */

  if (las[hnd].swap)
    {
      libslas_swap_uint32_t ((uint32_t *) &x);
      libslas_swap_uint32_t ((uint32_t *) &y);
      libslas_swap_uint32_t ((uint32_t *) &z);
      libslas_swap_uint16_t (&record->intensity);
      libslas_swap_uint16_t (&record->point_source_id);

      switch (las[hnd].header.point_data_format_id)
        {
        case 1:
          libslas_swap_double (&record->gps_time);
          break;

        case 2:
          libslas_swap_uint16_t (&record->red);
          libslas_swap_uint16_t (&record->green);
          libslas_swap_uint16_t (&record->blue);
          break;

        case 3:
          libslas_swap_double (&record->gps_time);
          libslas_swap_uint16_t (&record->red);
          libslas_swap_uint16_t (&record->green);
          libslas_swap_uint16_t (&record->blue);
          break;
        }
    }


  /*  Now put the rest of the data into the structure.  */

  record->x = ((double) x * las[hnd].header.x_scale_factor) + las[hnd].header.x_offset;
  record->y = ((double) y * las[hnd].header.y_scale_factor) + las[hnd].header.y_offset;
  record->z = (float) (((double) z * las[hnd].header.z_scale_factor) + las[hnd].header.z_offset);
  record->return_number = rets & 0x03;
  record->number_of_returns = (rets & 0x38) >> 3;
  record->edge_of_flightline = (rets & 0x40) >> 6;
  record->scan_direction_flag = (rets & 0x80) >> 7;
  record->classification = cls & 0x1f;
  record->synthetic = (cls & 0x20) >> 5;
  record->key_point = (cls & 0x40) >> 6;
  record->withheld = (cls & 0x80) >> 7;

  las[hnd].at_end = 0;
  las[hnd].modified = 0;
  las[hnd].write = 0;


  return (libslas_error.libslas = LIBSLAS_SUCCESS);
}




/********************************************************************************************/
/*!

 - Function:    libslas_append_point_data

 - Purpose:     Append a LAS point data record.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        07/16/14

 - Arguments:
                - hnd            =    The file handle
                - record         =    The LIBSLAS_POINT_DATA structure to be written

 - Returns:
                - LIBSLAS_SUCCESS
                - LIBSLAS_APPEND_ERROR
                - LIBSLAS_VALUE_OUT_OF_RANGE_ERROR
                - LIBSLAS_WRITE_ERROR

 - Caveats:     All returned error values are less than zero.  Success or a file handle
                will be greater than or equal to zero.  A simple test for failure is to
                check to see if the return is less than zero.

                This function is only used to append records when creating a file.  DO NOT
                try to use it to update fields in the point data record.  It won't work.
                Not only that but, if you were to read and write the x, y, and z fields
                over and over you would continuously be aliasing them since they're scaled
                offset integers.  The round off would cause data "creep".

*********************************************************************************************/

int32_t libslas_append_point_data (int32_t hnd, LIBSLAS_POINT_DATA *record)
{
  int32_t  x, y, z, pos;
  uint8_t  data[POINT_DATA_SIZE], rets, cls;


  /*  Appending a record is only allowed if you are creating a new file.  */

  if (!las[hnd].created)
    {
      sprintf (libslas_error.info, _("File : %s\nAppending to pre-existing LAS file not allowed.\nFunction: %s, Line: %d\n"), las[hnd].path,
               __FUNCTION__, __LINE__ - 3);
      return (libslas_error.libslas = LIBSLAS_APPEND_ERROR);
    }


  las[hnd].at_end = 1;


  /*  Check for min and max x, y, and z since we're appending a record.  */

  las[hnd].header.min_x = MIN (las[hnd].header.min_x, record->x);
  las[hnd].header.max_x = MAX (las[hnd].header.max_x, record->x);
  las[hnd].header.min_y = MIN (las[hnd].header.min_y, record->y);
  las[hnd].header.max_y = MAX (las[hnd].header.max_y, record->y);
  las[hnd].header.min_z = MIN (las[hnd].header.min_z, record->z);
  las[hnd].header.max_z = MAX (las[hnd].header.max_z, record->z);


  /*  Increment the number of records counter in the header.  */

  las[hnd].header.number_of_point_records++;


  /*  Check the return number.  */

  if (record->return_number < 1 || record->return_number > 5)
    {
      sprintf (libslas_error.info, _("File : %s\nReturn number %d is out of range (1-5).\nFunction: %s, Line: %d\n"), las[hnd].path, record->return_number,
               __FUNCTION__, __LINE__ - 3);
      return (libslas_error.libslas = LIBSLAS_RETURN_NUMBER_OUT_OF_RANGE_ERROR);
    }


  /*  Increment the number of points by return.  */

  las[hnd].header.number_of_points_by_return[record->return_number - 1]++;


  /*  Get the data out of the structure.  */

  x = NINT64 ((record->x - las[hnd].header.x_offset) / las[hnd].header.x_scale_factor);
  y = NINT64 ((record->y - las[hnd].header.y_offset) / las[hnd].header.y_scale_factor);
  z = NINT64 ((record->z - las[hnd].header.z_offset) / las[hnd].header.z_scale_factor);


  /*  Pack the bit fields.  */

  rets = 0;
  rets |= record->return_number;
  rets |= (record->number_of_returns << 3);
  rets |= (record->edge_of_flightline << 6);
  rets |= (record->scan_direction_flag << 7);
  cls = 0;
  cls |= record->classification;
  cls |= (record->withheld << 7);
  cls |= (record->key_point << 6);
  cls |= (record->synthetic << 5);


  /*  If we have to swap the record, do so.  */

  if (las[hnd].swap)
    {
      libslas_swap_uint32_t ((uint32_t *) &x);
      libslas_swap_uint32_t ((uint32_t *) &y);
      libslas_swap_uint32_t ((uint32_t *) &z);
      libslas_swap_uint16_t (&record->intensity);
      libslas_swap_uint16_t (&record->point_source_id);

      switch (las[hnd].header.point_data_format_id)
        {
        case 1:
          libslas_swap_double (&record->gps_time);
          break;

        case 2:
          libslas_swap_uint16_t (&record->red);
          libslas_swap_uint16_t (&record->green);
          libslas_swap_uint16_t (&record->blue);
          break;

        case 3:
          libslas_swap_double (&record->gps_time);
          libslas_swap_uint16_t (&record->red);
          libslas_swap_uint16_t (&record->green);
          libslas_swap_uint16_t (&record->blue);
          break;
        }
    }


  /*  Put the data into the buffer.  */

  pos = 0;
  memcpy (&data[pos], &x, 4); pos += 4;
  memcpy (&data[pos], &y, 4); pos += 4;
  memcpy (&data[pos], &z, 4); pos += 4;
  memcpy (&data[pos], &record->intensity, 2); pos += 2;
  memcpy (&data[pos], &rets, 1); pos += 1;
  memcpy (&data[pos], &cls, 1); pos += 1;
  memcpy (&data[pos], &record->scan_angle_rank, 1); pos += 1;
  memcpy (&data[pos], &record->user_data, 1); pos += 1;
  memcpy (&data[pos], &record->point_source_id, 2); pos += 2;


  switch (las[hnd].header.point_data_format_id)
    {
    case 1:
      memcpy (&data[pos], &record->gps_time, 8); pos += 8;
      break;

    case 2:
      memcpy (&data[pos], &record->red, 2); pos += 2;
      memcpy (&data[pos], &record->green, 2); pos += 2;
      memcpy (&data[pos], &record->blue, 2); pos += 2;
      break;

    case 3:
      memcpy (&data[pos], &record->gps_time, 8); pos += 8;
      memcpy (&data[pos], &record->red, 2); pos += 2;
      memcpy (&data[pos], &record->green, 2); pos += 2;
      memcpy (&data[pos], &record->blue, 2); pos += 2;
      break;
    }


  /*  Write the record.  */

  if (!fwrite (data, las[hnd].header.point_data_record_length, 1, las[hnd].fp))
    {
      sprintf (libslas_error.info, _("File : %s\nError writing LAS record :\n%s\nFunction: %s, Line: %d\n"), las[hnd].path, strerror (errno),
               __FUNCTION__, __LINE__ - 3);
      return (libslas_error.libslas = LIBSLAS_WRITE_ERROR);
    }


  /*  Set the current position.  */

  las[hnd].pos = ftello64 (las[hnd].fp);


  las[hnd].at_end = 1;
  las[hnd].modified = 1;
  las[hnd].write = 1;
  las[hnd].data = 1;


  return (libslas_error.libslas = LIBSLAS_SUCCESS);
}



/********************************************************************************************/
/*!

 - Function:    libslas_update_point_data

 - Purpose:     Updates the user modifiable fields of a LAS point data record without affecting
                the "non-modifiable" fields.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        07/16/14

 - Arguments:
                - hnd            =    The file handle
                - recnum         =    The record number of the LAS point data record to be written
		                      (records start at 0)
                - record         =    The LIBSLAS_POINT_DATA structure to be updated

 - Returns:
                - LIBSLAS_SUCCESS
                - LIBSLAS_INVALID_RECORD_NUMBER_ERROR
                - LIBSLAS_UPDATE_FSEEK_ERROR
                - LIBSLAS_UPDATE_ERROR
                - LIBSLAS_UPDATE_READ_ERROR
                - LIBSLAS_NOT_OPEN_FOR_UPDATE_ERROR

 - Caveats:     All returned error values are less than zero.  Success or a file handle
                will be greater than or equal to zero.  A simple test for failure is to
                check to see if the return is less than zero.

*********************************************************************************************/

int32_t libslas_update_point_data (int32_t hnd, int32_t recnum, LIBSLAS_POINT_DATA *record)
{
  int32_t   pos;
  uint16_t  psid, red, green, blue;
  uint8_t   data[POINT_DATA_SIZE], cls;
  int64_t   addr;


  /*  Check for LIBSLAS_UPDATE mode.  */

  if (las[hnd].mode != LIBSLAS_UPDATE)
    {
      sprintf (libslas_error.info, _("File : %s\nNot opened for update.\nFunction: %s, Line: %d\n"), las[hnd].path,
               __FUNCTION__, __LINE__ - 3);
      return (libslas_error.libslas = LIBSLAS_NOT_OPEN_FOR_UPDATE_ERROR);
    }


  /*  Check for record out of bounds.  */

  if (recnum >= las[hnd].header.number_of_point_records || recnum < 0)
    {
      sprintf (libslas_error.info, _("File : %s\nRecord : %d\nInvalid record number.\nFunction: %s, Line: %d\n"), las[hnd].path, recnum,
               __FUNCTION__, __LINE__ - 3);
      return (libslas_error.libslas = LIBSLAS_INVALID_RECORD_NUMBER_ERROR);
    }


  /*  Seek to the record.  */

  addr = (int64_t) las[hnd].header.offset_to_point_data + (int64_t) las[hnd].header.point_data_record_length * (int64_t) recnum;


  if (fseeko64 (las[hnd].fp, addr, SEEK_SET) < 0)
    {
      sprintf (libslas_error.info, _("File : %s\nError during fseek prior to updating LAS record :\n%s\nFunction: %s, Line: %d\n"), las[hnd].path, strerror (errno),
               __FUNCTION__, __LINE__ - 3);
      return (libslas_error.libslas = LIBSLAS_UPDATE_FSEEK_ERROR);
    }


  /*  Read the record.  */

  if (!fread (data, las[hnd].header.point_data_record_length, 1, las[hnd].fp))
    {
      sprintf (libslas_error.info, _("File : %s\nRecord : %d\nError reading LAS record :\n%s\nFunction: %s, Line: %d\n"), las[hnd].path, recnum, strerror (errno),
               __FUNCTION__, __LINE__ - 3);
      return (libslas_error.libslas = LIBSLAS_UPDATE_READ_ERROR);
    }


  /*  Modify the fields that can be changed (see libslas.h)  */

  cls = 0;
  cls |= record->classification;
  cls |= (record->withheld << 7);
  cls |= (record->key_point << 6);
  cls |= (record->synthetic << 5);


  /*  Position to the classification field.  */

  pos = 15;
  memcpy (&data[pos], &cls, 1); pos += 1;


  /*  Move past the scan_angle_rank field.  */

  pos +=1;
  memcpy (&data[pos], &record->user_data, 1); pos += 1;


  /*  Swap the point source ID if needed.  */

  psid = record->point_source_id;
  if (las[hnd].swap) libslas_swap_uint16_t (&psid);
  memcpy (&data[pos], &psid, 2); pos += 2;


  red = record->red;
  green = record->green;
  blue = record->blue;

  if (las[hnd].header.point_data_format_id > 1)
    {
      /*  If we're using format 3, move past the GPS time.  */

      if (las[hnd].header.point_data_format_id == 3) pos += 8;


      /*  Swap if we have to.  */

      if (las[hnd].swap)
        {
          libslas_swap_uint16_t (&red);
          libslas_swap_uint16_t (&green);
          libslas_swap_uint16_t (&blue);
        }

      memcpy (&data[pos], &red, 2); pos += 2;
      memcpy (&data[pos], &green, 2); pos += 2;
      memcpy (&data[pos], &blue, 2); pos += 2;
    }


  /*  Go back to the beginning of the record.  */

  if (fseeko64 (las[hnd].fp, addr, SEEK_SET) < 0)
    {
      sprintf (libslas_error.info, _("File : %s\nError during fseek prior to updating LAS record :\n%s\nFunction: %s, Line: %d\n"), las[hnd].path, strerror (errno),
               __FUNCTION__, __LINE__ - 3);
      return (libslas_error.libslas = LIBSLAS_UPDATE_FSEEK_ERROR);
    }


  /*  Write the record.  */

  if (!fwrite (data, las[hnd].header.point_data_record_length, 1, las[hnd].fp))
    {
      sprintf (libslas_error.info, _("File : %s\nError writing LAS record :\n%s\nFunction: %s, Line: %d\n"), las[hnd].path, strerror (errno),
               __FUNCTION__, __LINE__ - 3);
      return (libslas_error.libslas = LIBSLAS_WRITE_ERROR);
    }


  /*  Set the current position.  */

  las[hnd].pos = ftello64 (las[hnd].fp);


  las[hnd].at_end = 0;
  las[hnd].modified = 1;
  las[hnd].write = 1;


  return (libslas_error.libslas = LIBSLAS_SUCCESS);
}



/********************************************************************************************/
/*!

 - Function:    libslas_strerror

 - Purpose:     Returns the error string related to the latest error.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        07/16/14

 - Arguments:
                - void

 - Returns:
                - Error message

*********************************************************************************************/

char *libslas_strerror ()
{
  return (libslas_error.info);
}



/********************************************************************************************/
/*!

 - Function:    libslas_perror

 - Purpose:     Prints (to stderr) the latest error messages.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        07/16/14

 - Arguments:
                - void

 - Returns:
                - void

*********************************************************************************************/

void libslas_perror ()
{
  fprintf (stderr, "%s", libslas_strerror ());
  fflush (stderr);
}



/********************************************************************************************/
/*!

 - Function:    libslas_dump_las_header

 - Purpose:     Print the LAS header.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        07/16/14

 - Arguments:
                - header         =    The LIBSLAS_HEADER structure
                - fp             =    The file pointer where you want to dump the record.
                                      This can be stderr, stdout, or a file that you specify.

 - Returns:
                - void

*********************************************************************************************/

void libslas_dump_las_header (LIBSLAS_HEADER *header, FILE *fp)
{
  int32_t i;

  fprintf (fp, N_("\n******************************************************************\n"));
  fprintf (fp, _("File source ID : %d\n"), header->file_source_id);
  fprintf (fp, _("Global encoding : %x\n"), header->global_encoding);
  fprintf (fp, _("Project ID GUID data 1 : %d\n"), header->GUID_data_1);
  fprintf (fp, _("Project ID GUID data 2 : %d\n"), header->GUID_data_2);
  fprintf (fp, _("Project ID GUID data 3 : %d\n"), header->GUID_data_3);
  fprintf (fp, _("Project ID GUID data 4 : %s\n"), header->GUID_data_4);
  fprintf (fp, _("Version major : %d\n"), header->version_major);
  fprintf (fp, _("Version minor : %d\n"), header->version_minor);
  fprintf (fp, _("System ID : %s\n"), header->system_id);
  fprintf (fp, _("Generating software : %s\n"), header->generating_software);
  fprintf (fp, _("File creation day of year : %d\n"), header->file_creation_DOY);
  fprintf (fp, _("File year : %d\n"), header->file_creation_year);
  fprintf (fp, _("Header size : %d\n"), header->header_size);
  fprintf (fp, _("Offset to point data : %d\n"), header->offset_to_point_data);
  fprintf (fp, _("Number of variable length records : %d\n"), header->number_of_VLRs);
  fprintf (fp, _("Point data format ID : %d\n"), header->point_data_format_id);
  fprintf (fp, _("Point data record length : %d\n"), header->point_data_record_length);
  fprintf (fp, _("Number of point records : %d\n"), header->number_of_point_records);
  for (i = 0 ; i < 5 ; i++) fprintf (fp, _("Number of points for return %d : %d\n"), i + 1, header->number_of_points_by_return[i]);
  fprintf (fp, _("X scale factor : %.11f\n"), header->x_scale_factor);
  fprintf (fp, _("Y scale factor : %.11f\n"), header->y_scale_factor);
  fprintf (fp, _("Z scale factor : %.11f\n"), header->z_scale_factor);
  fprintf (fp, _("X offset : %.11f\n"), header->x_offset);
  fprintf (fp, _("Y offset : %.11f\n"), header->y_offset);
  fprintf (fp, _("Z offset : %.11f\n"), header->z_offset);
  fprintf (fp, _("Max X : %.11f\n"), header->max_x);
  fprintf (fp, _("Min X : %.11f\n"), header->min_x);
  fprintf (fp, _("Max Y : %.11f\n"), header->max_y);
  fprintf (fp, _("Min Y : %.11f\n"), header->min_y);
  fprintf (fp, _("Max Z : %.11f\n"), header->max_z);
  fprintf (fp, _("Min Z : %.11f\n"), header->min_z);

  fflush (fp);
}



/********************************************************************************************/
/*!

 - Function:    libslas_dump_vlr_header

 - Purpose:     Print the VLR header.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        07/16/14

 - Arguments:
                - vlr_header     =    The LIBSLAS_VLR_HEADER structure
                - fp             =    The file pointer where you want to dump the record.
                                      This can be stderr, stdout, or a file that you specify.

 - Returns:
                - void

*********************************************************************************************/

void libslas_dump_vlr_header (LIBSLAS_VLR_HEADER *vlr_header, FILE *fp)
{
  fprintf (fp, N_("\n******************************************************************\n"));
  fprintf (fp, _("Reserved : %d\n"), vlr_header->reserved);
  fprintf (fp, _("User ID : %s\n"), vlr_header->user_id);
  fprintf (fp, _("Record ID : %d\n"), vlr_header->record_id);
  fprintf (fp, _("Record length after header : %d\n"), vlr_header->record_length_after_header);
  fprintf (fp, _("Description : %s\n"), vlr_header->description);

  fflush (fp);
}



/********************************************************************************************/
/*!

 - Function:    libslas_dump_point_data

 - Purpose:     Print the point data.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        07/16/14

 - Arguments:
                - record         =    The LIBSLAS_POINT_DATA structure
                - fp             =    The file pointer where you want to dump the record.
                                      This can be stderr, stdout, or a file that you specify.

 - Returns:
                - void

*********************************************************************************************/

void libslas_dump_point_data (LIBSLAS_POINT_DATA *record, FILE *fp)
{
  fprintf (fp, N_("\n******************************************************************\n"));
  fprintf (fp, _("X : %.11f\n"), record->x);
  fprintf (fp, _("Y : %.11f\n"), record->y);
  fprintf (fp, _("Z : %.11f\n"), record->z);
  fprintf (fp, _("Intensity : %d\n"), record->intensity);
  fprintf (fp, _("Return number : %d\n"), record->return_number);
  fprintf (fp, _("Number of returns : %d\n"), record->number_of_returns);
  fprintf (fp, _("Scan direction flag : %x\n"), record->scan_direction_flag);
  fprintf (fp, _("Edge of flightline : %x\n"), record->edge_of_flightline);
  fprintf (fp, _("Classification : %d\n"), record->classification);
  fprintf (fp, _("Withheld bit : %x\n"), record->withheld);
  fprintf (fp, _("Key point bit : %x\n"), record->key_point);
  fprintf (fp, _("Synthetic bit : %x\n"), record->synthetic);
  fprintf (fp, _("Scan angle rank : %d\n"), record->scan_angle_rank);
  fprintf (fp, _("User data : %d\n"), record->user_data);
  fprintf (fp, _("Point source ID : %d\n"), record->point_source_id);
  fprintf (fp, _("GPS time : %.7f\n"), record->gps_time);
  fprintf (fp, _("Red : %d\n"), record->red);
  fprintf (fp, _("Green : %d\n"), record->green);
  fprintf (fp, _("Blue : %d\n"), record->blue);

  fflush (fp);
}
