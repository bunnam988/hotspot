/************************************************************************************
  If not stated otherwise in this file or this component's Licenses.txt file the
  following copyright and licenses apply:

  Copyright 2018 RDK Management

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
**************************************************************************/
/*********************************************************************************

    description:

        This is the template file of ssp_main.c for XxxxSsp.
        Please replace "XXXX" with your own ssp name with the same up/lower cases.

  ------------------------------------------------------------------------------

    revision:

        09/08/2011    initial revision.

**********************************************************************************/
#ifdef __GNUC__
#ifndef _BUILD_ANDROID
#include <execinfo.h>
#endif
#endif

#include "ssp_global.h"
#include "stdlib.h"
#include "ccsp_dm_api.h"
#include "hotspotfd.h"
#include "secure_wrapper.h"
#include "safec_lib_common.h"
#include <telemetry_busmessage_sender.h>

#ifdef INCLUDE_BREAKPAD
#include "breakpad_wrapper.h"
#endif
#define DEBUG_INI_NAME "/etc/debug.ini"
#include "cap.h"
static cap_user appcaps;

extern char*                                pComponentName;
char                                        g_Subsystem[32]         = {0};
int consoleDebugEnable = 0;
FILE* debugLogFile;

int  cmd_dispatch(int  command)
{
    switch ( command )
    {
        case    'e' :
            CcspTraceInfo(("Connect to bus daemon...\n"));
            {
                char                            CName[256];
                if ( g_Subsystem[0] != 0 )
                {
                    _ansc_sprintf(CName, "%s%s", g_Subsystem, CCSP_COMPONENT_ID_HOTSPOT);
                }
                else
                {
                    _ansc_sprintf(CName, "%s", CCSP_COMPONENT_ID_HOTSPOT);
                }
                ssp_Mbi_MessageBusEngage(CName, CCSP_MSG_BUS_CFG, CCSP_COMPONENT_PATH_HOTSPOT);
            }
            ssp_create();
            ssp_engage();
            break;
        
		case    'm':
	        AnscPrintComponentMemoryTable(pComponentName);
            break;
    
	    case    't':
        	AnscTraceMemoryTable();
           	break;

        case    'c':                
        	ssp_cancel();
            break;

        default:
            break;
    }
    return 0;
}

static void _print_stack_backtrace(void)
{
#ifdef __GNUC__
#ifndef _BUILD_ANDROID
	void* tracePtrs[100];
	char** funcNames = NULL;
	int i, count = 0;

	count = backtrace( tracePtrs, 100 );
	backtrace_symbols_fd( tracePtrs, count, 2 );

	funcNames = backtrace_symbols( tracePtrs, count );

	if ( funcNames ) {
            // Print the stack trace
	    for( i = 0; i < count; i++ )
		printf("%s\n", funcNames[i] );

            // Free the string pointers
            free( funcNames );
	}
#endif
#endif
}

static void daemonize(void) {
	switch (fork()) {
	case 0:
		break;
	case -1:
		// Error
		CcspTraceInfo(("Error daemonizing (fork)! %d - %s\n", errno, strerror(
				errno)));
		exit(0);
		break;
	default:
		_exit(0);
	}

	if (setsid() < 	0) {
		CcspTraceInfo(("Error demonizing (setsid)! %d - %s\n", errno, strerror(errno)));
		exit(0);
	}
#ifndef  _DEBUG
	int fd;
	fd = open("/dev/null", O_RDONLY);
	if (fd != 0) {
		dup2(fd, 0);
		close(fd);
	}
	fd = open("/dev/null", O_WRONLY);
	if (fd != 1) {
		dup2(fd, 1);
		close(fd);
	}
	fd = open("/dev/null", O_WRONLY);
	if (fd != 2) {
		dup2(fd, 2);
		close(fd);
	}
#endif
}

void sig_handler(int sig)
{
    if ( sig == SIGINT ) {
    	signal(SIGINT, sig_handler); /* reset it to this function */
    	CcspTraceInfo(("SIGINT received!\n"));
	exit(0);
    }
    else if ( sig == SIGUSR1 ) {
    	signal(SIGUSR1, sig_handler); /* reset it to this function */
    	CcspTraceInfo(("SIGUSR1 received!\n"));
    }
    else if ( sig == SIGUSR2 ) {
    	CcspTraceInfo(("SIGUSR2 received!\n"));
    }
    else if ( sig == SIGCHLD ) {
    	signal(SIGCHLD, sig_handler); /* reset it to this function */
    	CcspTraceInfo(("SIGCHLD received!\n"));
    }
    else if ( sig == SIGPIPE ) {
    	signal(SIGPIPE, sig_handler); /* reset it to this function */
    	CcspTraceInfo(("SIGPIPE received!\n"));
    }
    else {
    	/* get stack trace first */
    	_print_stack_backtrace();
    	CcspTraceInfo(("Signal %d received, exiting!\n", sig));
    	exit(0);
    }

}

static bool drop_root()
{
    bool retval = false;
    CcspTraceInfo(("NonRoot feature is enabled, dropping root privileges for CcspHotspot process\n"));
    appcaps.caps = NULL;
    appcaps.user_name = NULL;

    if(init_capability() != NULL) {
        if(drop_root_caps(&appcaps) != -1) {
            if(update_process_caps(&appcaps) != -1) {
                read_capability(&appcaps);
                retval = true;
            }
        }
    }
    return retval;
}

// Log flooding thread function for test purposes
static void* log_flood_thread(void* arg) {
    (void)arg;  // Suppress unused parameter warning
    CcspTraceInfo(("[DEBUG] Log flooding code changes are present and running.\n"));
    while (1) {
        int mode = 0;
        if (access("/tmp/log_flood", F_OK) == 0) {
            mode = 1;  // Single log burst (~100 msg/s for 5s)
        } else if (access("/tmp/log_pattern_2", F_OK) == 0) {
            mode = 2;
        } else if (access("/tmp/log_pattern_3", F_OK) == 0) {
            mode = 3;
        } else if (access("/tmp/log_pattern_4", F_OK) == 0) {
            mode = 4;
        } else if (access("/tmp/log_pattern_5", F_OK) == 0) {
            mode = 5;
        } else if (access("/tmp/log_pattern_6", F_OK) == 0) {
            mode = 6;
        } else if (access("/tmp/log_pattern_7", F_OK) == 0) {
            mode = 7;
        } else if (access("/tmp/log_pattern_8", F_OK) == 0) {
            mode = 8;
        } else if (access("/tmp/log_pattern_9", F_OK) == 0) {
            mode = 9;
        } else if (access("/tmp/log_pattern_10", F_OK) == 0) {
            mode = 10;
        } else if (access("/tmp/log_periodic", F_OK) == 0) {
            mode = 11; // Periodic: same msg every 2s for 30s
        } else if (access("/tmp/log_periodic_pattern", F_OK) == 0) {
            mode = 12; // Periodic pattern: 3-msg pattern every 3s for 30s
        } else if (access("/tmp/log_sporadic", F_OK) == 0) {
            mode = 13; // Sporadic: same msg at random intervals for 30s
        } else if (access("/tmp/log_short_burst", F_OK) == 0) {
            mode = 14; // Short burst: 50 msgs in 200ms then stop
        } else if (access("/tmp/log_accelerating", F_OK) == 0) {
            mode = 15; // Accelerating: starts slow, gets faster
        }
        if (mode != 0) {
            CcspTraceInfo(("[DEBUG] Entered log flooding logic, mode=%d\n", mode));
        }
        if (mode == 1) {
            // Burst: Flood same log for 5 sec at ~100 msg/s
            time_t start = time(NULL);
            while (difftime(time(NULL), start) < 5.0) {
                CcspTraceInfo(("[LOG FLOOD] Test log flooding\n"));
                usleep(10000); // 10ms between logs
            }
            CcspTraceInfo(("[LOG FLOOD] Flood complete, different message\n"));
        } else if (mode >= 2 && mode <= 10) {
            // Burst pattern: N-message pattern at ~100 msg/s for 5 sec
            const char *patterns[11][10] = {
                {},  // mode 0 - unused
                {},  // mode 1 - handled separately
                {"[LOG PATTERN 2] Log 1\n", "[LOG PATTERN 2] Log 2\n"},
                {"[LOG PATTERN 3] Log 1\n", "[LOG PATTERN 3] Log 2\n", "[LOG PATTERN 3] Log 3\n"},
                {"[LOG PATTERN 4] Log 1\n", "[LOG PATTERN 4] Log 2\n", "[LOG PATTERN 4] Log 3\n", "[LOG PATTERN 4] Log 4\n"},
                {"[LOG PATTERN 5] Log 1\n", "[LOG PATTERN 5] Log 2\n", "[LOG PATTERN 5] Log 3\n", "[LOG PATTERN 5] Log 4\n", "[LOG PATTERN 5] Log 5\n"},
                {"[LOG PATTERN 6] Log 1\n", "[LOG PATTERN 6] Log 2\n", "[LOG PATTERN 6] Log 3\n", "[LOG PATTERN 6] Log 4\n", "[LOG PATTERN 6] Log 5\n", "[LOG PATTERN 6] Log 6\n"},
                {"[LOG PATTERN 7] Log 1\n", "[LOG PATTERN 7] Log 2\n", "[LOG PATTERN 7] Log 3\n", "[LOG PATTERN 7] Log 4\n", "[LOG PATTERN 7] Log 5\n", "[LOG PATTERN 7] Log 6\n", "[LOG PATTERN 7] Log 7\n"},
                {"[LOG PATTERN 8] Log 1\n", "[LOG PATTERN 8] Log 2\n", "[LOG PATTERN 8] Log 3\n", "[LOG PATTERN 8] Log 4\n", "[LOG PATTERN 8] Log 5\n", "[LOG PATTERN 8] Log 6\n", "[LOG PATTERN 8] Log 7\n", "[LOG PATTERN 8] Log 8\n"},
                {"[LOG PATTERN 9] Log 1\n", "[LOG PATTERN 9] Log 2\n", "[LOG PATTERN 9] Log 3\n", "[LOG PATTERN 9] Log 4\n", "[LOG PATTERN 9] Log 5\n", "[LOG PATTERN 9] Log 6\n", "[LOG PATTERN 9] Log 7\n", "[LOG PATTERN 9] Log 8\n", "[LOG PATTERN 9] Log 9\n"},
                {"[LOG PATTERN 10] Log 1\n", "[LOG PATTERN 10] Log 2\n", "[LOG PATTERN 10] Log 3\n", "[LOG PATTERN 10] Log 4\n", "[LOG PATTERN 10] Log 5\n", "[LOG PATTERN 10] Log 6\n", "[LOG PATTERN 10] Log 7\n", "[LOG PATTERN 10] Log 8\n", "[LOG PATTERN 10] Log 9\n", "[LOG PATTERN 10] Log 10\n"}
            };
            time_t start = time(NULL);
            int pattern_idx = 0;
            while (difftime(time(NULL), start) < 5.0) {
                CcspTraceInfo(("%s", patterns[mode][pattern_idx]));
                pattern_idx = (pattern_idx + 1) % mode;
                usleep(10000); // 10ms between logs
            }
            // Print a different message after pattern flood to break the pattern
            CcspTraceInfo(("[LOG PATTERN %d] Flood complete, pattern broken for summary\n", mode));
        } else if (mode == 11) {
            // Periodic: same message every 2 seconds for 30 seconds
            // Expected: classification = "periodic ~every 2s"
            time_t start = time(NULL);
            while (difftime(time(NULL), start) < 30.0) {
                CcspTraceInfo(("[PERIODIC TEST] Health check ping\n"));
                sleep(2);
            }
            CcspTraceInfo(("[PERIODIC TEST] Done, breaking pattern\n"));
        } else if (mode == 12) {
            // Periodic pattern: 3-message pattern every 3 seconds for 30 seconds
            // Internal gaps are uneven (A immediately, B after 100ms, C after 200ms)
            // Expected: classification = "periodic ~every 3s" (tracks cycle-to-cycle)
            time_t start = time(NULL);
            while (difftime(time(NULL), start) < 30.0) {
                CcspTraceInfo(("[PERIODIC PAT] Step A\n"));
                usleep(100000); // 100ms
                CcspTraceInfo(("[PERIODIC PAT] Step B\n"));
                usleep(200000); // 200ms
                CcspTraceInfo(("[PERIODIC PAT] Step C\n"));
                // Wait remainder of 3s cycle
                usleep(2700000); // 2.7s (total cycle = 3s)
            }
            CcspTraceInfo(("[PERIODIC PAT] Done, breaking pattern\n"));
        } else if (mode == 13) {
            // Sporadic: same message at random-ish intervals (1-8 seconds) for 60 seconds
            // Expected: classification = "sporadic over Xs"
            time_t start = time(NULL);
            unsigned int seed = (unsigned int)time(NULL);
            while (difftime(time(NULL), start) < 60.0) {
                CcspTraceInfo(("[SPORADIC TEST] Random event occurred\n"));
                // Vary between 1-8 seconds (use simple PRNG to avoid rand() thread issues)
                seed = seed * 1103515245 + 12345;
                unsigned int delay_sec = 1 + (seed >> 16) % 8;
                sleep(delay_sec);
            }
            CcspTraceInfo(("[SPORADIC TEST] Done, breaking pattern\n"));
        } else if (mode == 14) {
            // Short burst: 50 messages in 200ms then stop
            // Expected: classification = "burst ~250 msg/s" with "at HH:MM:SS" (sub-second)
            int i;
            for (i = 0; i < 50; i++) {
                CcspTraceInfo(("[SHORT BURST] Rapid fire event\n"));
                usleep(4000); // 4ms = ~250 msg/s
            }
            CcspTraceInfo(("[SHORT BURST] Done, breaking pattern\n"));
        } else if (mode == 15) {
            // Accelerating: starts at 1 msg/s, ramps to 50 msg/s over 10 seconds
            // Expected: classification = "sporadic" (large ratio between min/max gaps)
            time_t start = time(NULL);
            while (difftime(time(NULL), start) < 10.0) {
                double elapsed = difftime(time(NULL), start);
                // Delay goes from 1000ms down to 20ms over 10 seconds
                int delay_ms = 1000 - (int)(elapsed * 98);
                if (delay_ms < 20) delay_ms = 20;
                CcspTraceInfo(("[ACCEL TEST] Repeated event\n"));
                usleep(delay_ms * 1000);
            }
            CcspTraceInfo(("[ACCEL TEST] Done, breaking pattern\n"));
        }
        sleep(30);
    }
    return NULL;
}

int main(int argc, char* argv[])
{
  
    BOOL bRunAsDaemon = TRUE;
    int cmdChar = 0;
    int idx = 0;
    errno_t rc = -1;
    int ind = -1;

    extern ANSC_HANDLE bus_handle;
    char *subSys = NULL;  
    DmErr_t err;

    // Buffer characters till newline for stdout and stderr
    setlinebuf(stdout);
    setlinebuf(stderr);

    debugLogFile = stderr;
  
    rdk_logger_init(DEBUG_INI_NAME);

    t2_init("ccsp-hotspot");

     for (idx = 1; idx < argc; idx++)
    {
                rc = strcmp_s("-subsys", strlen("-subsys"),argv[idx], &ind);      
                 ERR_CHK(rc);        
                 if ((ind == 0) && (rc == EOK))
                 {
                        /*Coverity Fix CID:135244 STRING_SIZE */
                         if( (idx+1) < argc )
                         {  
                            rc = strcpy_s(g_Subsystem, sizeof(g_Subsystem), argv[idx+1]);
			   
                            if(rc != EOK)
			   
                            {
				   
                                  ERR_CHK(rc);
				   
                                   return -1;
			
                             }
                         }
                          else
                          {
                              CcspTraceError(("Missing in -subsys \n"));
                               exit(0);
                          } 
            
                 }
                 else
                 {
			
                          rc = strcmp_s("-c", strlen("-c"),argv[idx], &ind);
            
                          ERR_CHK(rc);
           
                          if ((ind == 0) && (rc == EOK))
			
                          {
                              bRunAsDaemon = FALSE;
			
                          }
                           else
                          {
			    
                              rc = strcmp_s("-DEBUG", strlen("-DEBUG"),argv[idx], &ind);
                
                              ERR_CHK(rc);
                
                              if ((ind == 0) && (rc == EOK))
				
                              {
                                  consoleDebugEnable = 1;
                                   fprintf(debugLogFile, "DEBUG ENABLE ON\n");
                              }
                               else
                              { 
                                 rc = strcmp_s("-LOGFILE", strlen("-LOGFILE"),argv[idx], &ind);                   
                                 ERR_CHK(rc);
                                 if ((ind == 0) && (rc == EOK))
                                 {
                                       if( (idx+1) < argc )
                                       {          
                                           FILE *fp = fopen(argv[idx+1], "a+");
                                            if (! fp) 
                                            {
                                                 fprintf(debugLogFile, "Cannot open -LOGFILE %s\n", argv[idx+1]);
                                            } 
                                            else
                                            {
                                                   fclose(debugLogFile);
                                                    debugLogFile = fp;
                                                   fprintf(debugLogFile, "Log File [%s] Opened for Writing in Append Mode \n",  argv[idx+1]);
                                             }
      
                                        }
                    
                                   }
                   
                                }  
              
                           }
                     } 
     }

    pComponentName = CCSP_COMPONENT_NAME_HOTSPOT;

    if(!drop_root()) {
         CcspTraceInfo(("drop_root method failed!\n"));
    }

    if ( bRunAsDaemon ) 
        daemonize();

#ifdef INCLUDE_BREAKPAD
    breakpad_ExceptionHandler();
#else
    signal(SIGTERM, sig_handler);
    signal(SIGINT, sig_handler);
    /*signal(SIGCHLD, sig_handler);*/
    signal(SIGUSR1, sig_handler);
    signal(SIGUSR2, sig_handler);

    signal(SIGSEGV, sig_handler);
    signal(SIGBUS, sig_handler);
    signal(SIGKILL, sig_handler);
    signal(SIGFPE, sig_handler);
    signal(SIGILL, sig_handler);
    signal(SIGQUIT, sig_handler);
    signal(SIGHUP, sig_handler);
#endif  /*  INCLUDE_BREAKPAD */ 

    cmd_dispatch('e');
#ifdef _COSA_SIM_
    subSys = "";        /* PC simu use empty string as subsystem */
#else
    subSys = NULL;      /* use default sub-system */
#endif
    err = Cdm_Init(bus_handle, subSys, NULL, NULL, pComponentName);
    if (err != CCSP_SUCCESS)
    {
        fprintf(stderr, "Cdm_Init: %s\n", Cdm_StrError(err));
        exit(1);
    }
    rdk_logger_init(DEBUG_INI_NAME);
    v_secure_system("touch /tmp/hotspot_initialized");

    // Log flooding logic: always enabled in daemon mode
    if (bRunAsDaemon) {
        pthread_t log_flood_tid;
        pthread_create(&log_flood_tid, NULL, log_flood_thread, NULL);
    }

    hotspot_start();
    if (!bRunAsDaemon) {
        while (cmdChar != 'q') {
            cmdChar = getchar();
            cmd_dispatch(cmdChar);
        }
    }

	err = Cdm_Term();
	if (err != CCSP_SUCCESS)
	{
	fprintf(stderr, "Cdm_Term: %s\n", Cdm_StrError(err));
	exit(1);
	}

	ssp_cancel();

    if(debugLogFile)
    {
        fclose(debugLogFile);
    }
    
    return 0;
}

