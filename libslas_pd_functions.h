

/*  Please note that all of the software in this file is in the public domain.  */


/*********************************************************************************************

    This is public domain software that was developed by or for the U.S. Naval Oceanographic
    Office and/or the U.S. Army Corps of Engineers.

    This is a work of the U.S. Government. In accordance with 17 USC 105, copyright protection
    is not available for any work of the U.S. Government.

    Neither the United States Government, nor any employees of the United States Government,
    nor the author, makes any warranty, express or implied, without even the implied warranty
    of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE, or assumes any liability or
    responsibility for the accuracy, completeness, or usefulness of any information,
    apparatus, product, or process disclosed, or represents that its use would not infringe
    privately-owned rights. Reference herein to any specific commercial products, process,
    or service by trade name, trademark, manufacturer, or otherwise, does not necessarily
    constitute or imply its endorsement, recommendation, or favoring by the United States
    Government. The views and opinions of authors expressed herein do not necessarily state
    or reflect those of the United States Government, and shall not be used for advertising
    or product endorsement purposes.

*********************************************************************************************/


/*******************************************************************************************/
/*!

 - Function:    clean_exit

 - Purpose:     Exit from the application after first cleaning up memory and orphaned files.
                This will only be called in the case of an abnormal exit (like a failed malloc).

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        07/16/14

 - Arguments:
                - ret            =    Value to be passed to exit ();  If set to -999 we were
                                      called from the SIGINT signal handler so we must return
                                      to allow it to SIGINT itself.

 - Returns:
                - void

 - Caveats:     This function is static, it is only used internal to the API and is not
                callable from an external program.

*********************************************************************************************/

static void libslas_clean_exit (int32_t ret)
{
  int32_t i;

  for (i = 0 ; i < LIBSLAS_MAX_FILES ; i++)
    {
      /*  If we were in the process of creating a file we need to remove it since it isn't finished.  */

      if (las[i].fp != NULL)
        {
          if (las[i].created)
            {
              fclose (las[i].fp);
              remove (las[i].path);
            }
        }
    }


  /*  If we were called from the SIGINT handler return there, otherwise just exit.  */

  if (ret == -999 && getpid () > 1) return;


  exit (ret);
}



/*******************************************************************************************/
/*!

 - Function:    sigint_handler

 - Purpose:     Simple little SIGINT handler.  Allows us to clean up the files if we were 
                creating a new LIBSLAS file and someone does a CTRL-C.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        02/25/09

 - Arguments:
                - sig            =    The signal

 - Returns:
                - void

 - Caveats:     The way to do this was borrowed from Martin Cracauer's "Proper handling of
                SIGINT/SIGQUIT", http://www.cons.org/cracauer/sigint.html

                This function is static, it is only used internal to the API and is not
                callable from an external program.

*********************************************************************************************/

static void libslas_sigint_handler (int sig)
{
  libslas_clean_exit (-999);

  signal (SIGINT, SIG_DFL);

#ifdef NVWIN3X
  raise (sig);
#else
  kill (getpid (), SIGINT);
#endif
}



/***************************************************************************/
/*!
  - Module Name:        big_endian

  - Programmer(s):      Jan C. Depner

  - Date Written:       July 1992

  - Purpose:            This function checks to see if the system is
                        big-endian or little-endian.

  - Arguments:          NONE

  - Returns:            3 if big-endian, 0 if little-endian

\***************************************************************************/

static int32_t libslas_big_endian ()
{
    union
    {
        int32_t        word;
        uint8_t       byte[4];
    } a;

    a.word = 0x00010203;
    return ((int32_t) a.byte[3]);
}



/***************************************************************************/
/*!

  - Module Name:        swap_uint32_t

  - Programmer(s):      Jan C. Depner

  - Date Written:       July 1992

  - Purpose:            This function swaps bytes in a four byte int.

  - Arguments:          word                -   pointer to the int

****************************************************************************/

void libslas_swap_uint32_t (uint32_t *word)
{
    uint32_t    temp[4];

    temp[0] = *word & 0x000000ff;

    temp[1] = (*word & 0x0000ff00) >> 8;

    temp[2] = (*word & 0x00ff0000) >> 16;

    temp[3] = (*word & 0xff000000) >> 24;

    *word = (temp[0] << 24) + (temp[1] << 16) + (temp[2] << 8) + temp[3];
}



/***************************************************************************/
/*!

  - Module Name:        swap_double

  - Programmer(s):      Jan C. Depner

  - Date Written:       July 1992

  - Purpose:            This function swaps bytes in an eight byte double.

  - Arguments:          word                -   pointer to the double

****************************************************************************/

void libslas_swap_double (double *word)
{
    int32_t         i;
    uint8_t        temp;
    union
    {
        uint8_t    bytes[8];
        double   d;
    }eq;
    
    memcpy (&eq.bytes, word, 8);

    for (i = 0 ; i < 4 ; i++)
    {
        temp = eq.bytes[i];
        eq.bytes[i] = eq.bytes[7 - i];
        eq.bytes[7 - i] = temp;
    }

    *word = eq.d;
}



/***************************************************************************/
/*!

  - Module Name:        swap_uint16_t

  - Programmer(s):      Jan C. Depner

  - Date Written:       July 1992

  - Purpose:            This function swaps bytes in a two byte int.

  - Arguments:          word                -   pointer to the int

****************************************************************************/

void libslas_swap_uint16_t (uint16_t *word)
{
    uint16_t   temp;
#ifdef NVWIN3X
  #if defined (__MINGW32__) || defined (__MINGW64__)
    swab((char *)word, (char *)&temp, 2);
  #else
    _swab((char *)word, (char *)&temp, 2);
  #endif
#else
    swab (word, &temp, 2);
#endif
    *word = temp;

    return;
}
