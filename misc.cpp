/********************04/16/2002*********************
				    Misc.dll
					v0.0.1.0
			An Extension To Misc.ka
  Adds drive reporting functionality to kano through
  the use of newly created TCL procs with the WIN32
  API.  Of course there is always a better way to do
  something and if you find this way, feel free to
  drop us an e-mail.  You can use this straight
  with normal XiRCON, just do a load misc.dll
  then invoke the proc hdfree.

  Updates (04/16/2002): Sergio took the dll project.

  Copyright © 2002 Matt Saladna, Sergio Arizmendi
  nemisis@intercarve.net
  sergio@arizmendi.ws
***************************************************/ 	

#pragma warning( disable:  4244 )

#define VERSION "0.0.1.0"
#define DEFTITLE "DLL Addon For Misc.ka, a kano addon."
#define WIN32_LEAN_AND_MEAN
#define TCL_OK 0
#define TCL_ERROR 1

#include <windows.h>
#include <winbase.h>
#include <stdio.h>
#include <string>

/* TCL typedefs, for TCL usage without their library */
typedef int Tcl_CmdProc(void *cd, void *interp, int argc, char *argv[]);
typedef void (*dyn_CreateCommand)(void*, const char*, Tcl_CmdProc*, void*, void*);
typedef int (*dyn_AppendResult)(void*, ...);

/* Globals */
char *curstr = new char[];

// handle to this DLL, passed in to DllMain() and needed for several API functions
HINSTANCE hInstance;

// handle to xircntcl.dll, wherein resides TCL, which we need to call functions from
HINSTANCE hXircTcl;

// reference count, kept to know when we're being loaded the first time, or unloaded the last time
int refcount = 0;

// function pointers for the two functions we'll need to call in TCL
dyn_CreateCommand Tcl_CreateCommand;
dyn_AppendResult Tcl_AppendResult;


/* Prototypes */
bool ShowDriveType(char c);
void logicalDrives(char* &dd);
void diskSpaceFree(char *argv);

// debug!
void PUTDEBUG(char *errormsg) {
	MessageBoxEx(NULL,errormsg,"misc.dll debug:",MB_OK,LANG_ENGLISH);
}

// main dll
BOOL WINAPI DllMain(HINSTANCE hInst, DWORD dwReason, LPVOID lpReserved) {
	switch(dwReason) {
	case DLL_PROCESS_ATTACH:
		refcount++;
		if (refcount == 1) {
			hInstance = hInst; /* save instance handle */
			/* get handle to xircntcl for TCL functions. */
			hXircTcl = GetModuleHandle("xtcl.dll");

			if (hXircTcl) {
				Tcl_CreateCommand = (dyn_CreateCommand)GetProcAddress(hXircTcl, "_Tcl_CreateCommand");
				Tcl_AppendResult = (dyn_AppendResult)GetProcAddress(hXircTcl, "_Tcl_AppendResult");
			} else {
				Tcl_CreateCommand = NULL;
				Tcl_AppendResult = NULL;
			}
			/* need no data structures initialized, but if we did, this would
			   be the place */
		}
		break;
	case DLL_PROCESS_DETACH:
		refcount--;
		if (refcount == 0) {
			// anything to clean-up? <ctrl+b>YES<ctrl+b> Matt :)
			Tcl_CreateCommand = NULL;
			Tcl_AppendResult = NULL;
		}
		break;
	/* don't care about DLL_THREAD_* */
	}
	return TRUE; /* successful load */
}

/* Main Function */

int HDFree(void *cd, void *interp, int argc, char *argv[]) {
	/* needed to purge nasty symbols left over by instantiation */
	/* and also too, to prevent extensive string concatenation */

    ZeroMemory(curstr,sizeof(curstr));
	//PUTDEBUG(curstr);
	char *dd = new char[]; 
	logicalDrives(dd);
	char d[4] = " :\\"; 
	for(int i = 0; i < sizeof(dd); i++) {
		d[0] = dd[i];
		diskSpaceFree(d);
	}
	if (!Tcl_AppendResult)
		return TCL_OK;

	(*Tcl_AppendResult)(interp, curstr, NULL);
		return TCL_OK;
	//memset(curstr,'\0',100);
}


/* Fetch All Available Logical Drives */
void logicalDrives(char* &dd) {
	int n, j = -1;
	char drive[5] = " :\\\0";
	int drive_type;
	bool is_fixed_drive;
	DWORD dr = GetLogicalDrives();
	for( int i = 0; i < 26; i++ ) {
		n = ((dr>>i)&0x00000001);
		if( n == 1 ) { 
			drive[0] = char(65+i);
			drive_type = GetDriveType(drive);
			is_fixed_drive = drive_type!=DRIVE_NO_ROOT_DIR&&drive_type!=DRIVE_REMOVABLE;
			if (is_fixed_drive) { j++; dd[j] = drive[0]; }
		}
	}
}

/* Function To Calculate Disk Space Free Per Drive */
void diskSpaceFree(char *argv) {
     typedef BOOL (WINAPI *P_GDFSE)(LPCTSTR, PULARGE_INTEGER, 
                   PULARGE_INTEGER, PULARGE_INTEGER);
	
	  char buffer [sizeof(unsigned long)*8+1];
      BOOL  fResult;
	  char *tmpstr = new char[2];
      char  *pszDrive,
             szDrive[4];

      DWORD dwSectPerClust,
            dwBytesPerSect,
            dwFreeClusters,
            dwTotalClusters;
	  
	  char *units = new char[3];
	  units[1] = 'B';
	  P_GDFSE pGetDiskFreeSpaceEx = NULL;

      unsigned __int64 i64FreeBytesToCaller,
                       i64TotalBytes,
                       i64FreeBytes;

      /*
         Command line parsing.

         If the drive is a drive letter and not a UNC path, append a 
         trailing backslash to the drive letter and colon.  This is 
         required on Windows 95 and 98.
      */ 

      pszDrive = argv;

      if (pszDrive[1] == ':')
      {
         szDrive[0] = pszDrive[0];
         szDrive[1] = ':';
         szDrive[2] = '\\';
         szDrive[3] = '\0';

         pszDrive = szDrive;
      }
      /*
         Use GetDiskFreeSpaceEx if available; otherwise, use
         GetDiskFreeSpace.
         Note: Since GetDiskFreeSpaceEx is not in Windows 95 Retail, we
         dynamically link to it and only call it if it is present.  We 
         don't need to call LoadLibrary on KERNEL32.DLL because it is 
         already loaded into every Win32 process's address space.
      */ 
      pGetDiskFreeSpaceEx = (P_GDFSE)GetProcAddress (
                               GetModuleHandle ("kernel32.dll"),
                                                "GetDiskFreeSpaceExA");
      if (pGetDiskFreeSpaceEx)
      {
         fResult = pGetDiskFreeSpaceEx (pszDrive,
                                 (PULARGE_INTEGER)&i64FreeBytesToCaller,
                                 (PULARGE_INTEGER)&i64TotalBytes,
                                 (PULARGE_INTEGER)&i64FreeBytes);
         if (fResult)
         {
			 ultoa(i64FreeBytes/(1024 * 1024),buffer,10);
			 strcat(curstr,"[");
			 sprintf(tmpstr,"%c:",pszDrive[0]);
			 strcat(curstr,tmpstr);
			 strcat(curstr," \002Free\002: ");
			 strcat(curstr,buffer);
			 ultoa(i64TotalBytes/(1024 * 1024),buffer,10);
			 strcat(curstr," \037mB\037/\002Total\002: ");
			 strcat(curstr,buffer);
			 strcat(curstr," \037mB\037] ");

		 }
      }
      else if(!pGetDiskFreeSpaceEx)
      {
         fResult = GetDiskFreeSpace (pszDrive, 
                                     &dwSectPerClust,
                                     &dwBytesPerSect, 
                                     &dwFreeClusters,
                                     &dwTotalClusters);
         if (fResult) {
            i64TotalBytes = (__int64)dwTotalClusters * dwSectPerClust *
                              dwBytesPerSect;
            i64FreeBytes = (__int64)dwFreeClusters * dwSectPerClust *
                              dwBytesPerSect;
			 ultoa(i64FreeBytes/(1024 * 1024),buffer,10);
			 strcat(curstr,"[");
			 sprintf(tmpstr,"%c:",pszDrive[0]);
			 strcat(curstr,tmpstr);
			 strcat(curstr," \002Free\002: ");
			 strcat(curstr,buffer);
			 ultoa(i64TotalBytes/(1024 * 1024),buffer,10);
			 strcat(curstr," \037mB\037/\002Total\002: ");
			 strcat(curstr,buffer);
			 strcat(curstr," \037mB\037] ");
			}
	  }

      if (!fResult) { } // error generated, let's disregard it.
   		 
	}

// dll init export, run by xircons tcl interpreter when dll is loaded..
// append new tcl commands to xircons command structure
extern "C" {
	int __declspec(dllexport) Misc_Init(void *interp) {
		if (Tcl_CreateCommand && Tcl_AppendResult) {
			// register command with xircons tcl interpreter
			(*Tcl_CreateCommand)(interp, "hd_free", HDFree, NULL, NULL);
		}
		return TCL_OK;
	}
}

