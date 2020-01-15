// Minimal audio streaming using OpenSL.
//
// Loosely based on the Android NDK sample code.
// Hardcoded to 44.1kHz stereo 16-bit audio, because as far as I'm concerned,
// that's the only format that makes any sense.

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <syslog.h>

// for native audio
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

// This is kinda ugly, but for simplicity I've left these as globals just like in the sample,
// as there's not really any use case for this where we have multiple audio devices yet.

// engine interfaces
static SLObjectItf engineObject;
static SLEngineItf engineEngine;
static SLObjectItf outputMixObject;

// buffer queue player interfaces
static SLObjectItf bqPlayerObject = NULL;
static SLPlayItf bqPlayerPlay;
static SLAndroidSimpleBufferQueueItf bqPlayerBufferQueue;
static SLMuteSoloItf bqPlayerMuteSolo;
static SLVolumeItf bqPlayerVolume;

#define BUFFER_SIZE 512
#define BUFFER_SIZE_IN_SAMPLES (BUFFER_SIZE / 2)

// Double buffering.
static short buffer[2][BUFFER_SIZE];
static int curBuffer = 0;


// This callback handler is called every time a buffer finishes playing.
// The documentation available is very unclear about how to best manage buffers.
// I've chosen to this approach: Instantly enqueue a buffer that was rendered to the last time,
// and then render the next. Hopefully it's okay to spend time in this callback after having enqueued. 
static void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context) {
  assert(bq == bqPlayerBufferQueue);
  assert(NULL == context);

  short *nextBuffer = buffer[curBuffer];
  int nextSize = sizeof(buffer[0]);

  SLresult result;
  result = (*bqPlayerBufferQueue)->Enqueue(bqPlayerBufferQueue, nextBuffer, nextSize);

  // Comment from sample code:
  // the most likely other result is SL_RESULT_BUFFER_INSUFFICIENT,
  // which for this code example would indicate a programming error
  assert(SL_RESULT_SUCCESS == result);

  curBuffer ^= 1;  // Switch buffer
  // Render to the fresh buffer
  //audioCallback(buffer[curBuffer], BUFFER_SIZE_IN_SAMPLES);
}

// create the engine and output mix objects
extern "C" bool OpenSLWrap_Init() {
  SLresult result;
  // create engine
  result = slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);
  assert(SL_RESULT_SUCCESS == result);
  result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
  assert(SL_RESULT_SUCCESS == result);
  result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineEngine);
  assert(SL_RESULT_SUCCESS == result);
  result = (*engineEngine)->CreateOutputMix(engineEngine, &outputMixObject, 0, 0, 0);
  assert(SL_RESULT_SUCCESS == result);
  result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
  assert(SL_RESULT_SUCCESS == result);

  SLDataLocator_AndroidSimpleBufferQueue loc_bufq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
  SLDataFormat_PCM format_pcm = {
    SL_DATAFORMAT_PCM,
    2,
    SL_SAMPLINGRATE_44_1,
    SL_PCMSAMPLEFORMAT_FIXED_16,
    SL_PCMSAMPLEFORMAT_FIXED_16,
    SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,
    SL_BYTEORDER_LITTLEENDIAN
  };

  SLDataSource audioSrc = {&loc_bufq, &format_pcm};

  // configure audio sink
  SLDataLocator_OutputMix loc_outmix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};
  SLDataSink audioSnk = {&loc_outmix, NULL};

  // create audio player
  const SLInterfaceID ids[2] = {SL_IID_BUFFERQUEUE, SL_IID_VOLUME};
  const SLboolean req[2] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};
  result = (*engineEngine)->CreateAudioPlayer(engineEngine, &bqPlayerObject, &audioSrc, &audioSnk, 2, ids, req);
  assert(SL_RESULT_SUCCESS == result);

  result = (*bqPlayerObject)->Realize(bqPlayerObject, SL_BOOLEAN_FALSE);
  assert(SL_RESULT_SUCCESS == result);
  result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_PLAY, &bqPlayerPlay);
  assert(SL_RESULT_SUCCESS == result);
  result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_BUFFERQUEUE,
    &bqPlayerBufferQueue);
  assert(SL_RESULT_SUCCESS == result);
  result = (*bqPlayerBufferQueue)->RegisterCallback(bqPlayerBufferQueue, bqPlayerCallback, NULL);
  assert(SL_RESULT_SUCCESS == result);
  result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_VOLUME, &bqPlayerVolume);
  assert(SL_RESULT_SUCCESS == result);
  result = (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_PLAYING);
  assert(SL_RESULT_SUCCESS == result);

  // Render and enqueue a first buffer. (or should we just play the buffer empty?)
  curBuffer = 0;
  //audioCallback(buffer[curBuffer], BUFFER_SIZE_IN_SAMPLES);

  result = (*bqPlayerBufferQueue)->Enqueue(bqPlayerBufferQueue, buffer[curBuffer], sizeof(buffer[curBuffer]));
  if (SL_RESULT_SUCCESS != result) {
    return false;
  }
  curBuffer ^= 1;
  return true;
}

// shut down the native audio system
extern "C" void OpenSLWrap_Shutdown() {
  if (bqPlayerObject != NULL) {
    (*bqPlayerObject)->Destroy(bqPlayerObject);
    bqPlayerObject = NULL;
    bqPlayerPlay = NULL;
    bqPlayerBufferQueue = NULL;
    bqPlayerMuteSolo = NULL;
    bqPlayerVolume = NULL;
  }
  if (outputMixObject != NULL) {
    (*outputMixObject)->Destroy(outputMixObject);
    outputMixObject = NULL;
  }
  if (engineObject != NULL) {
    (*engineObject)->Destroy(engineObject);
    engineObject = NULL;
    engineEngine = NULL;
  }
}


static void skeleton_daemon()
{
    pid_t pid;

    /* Fork off the parent process */
    pid = fork();

    /* An error occurred */
    if (pid < 0)
        exit(EXIT_FAILURE);

    /* Success: Let the parent terminate */
    if (pid > 0)
        exit(EXIT_SUCCESS);

    /* On success: The child process becomes session leader */
    if (setsid() < 0)
        exit(EXIT_FAILURE);

    /* Catch, ignore and handle signals */
    //TODO: Implement a working signal handler */
    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);

    /* Fork off for the second time*/
    pid = fork();

    /* An error occurred */
    if (pid < 0)
        exit(EXIT_FAILURE);

    /* Success: Let the parent terminate */
    if (pid > 0)
        exit(EXIT_SUCCESS);

    /* Set new file permissions */
    umask(0);

    /* Change the working directory to the root directory */
    /* or another appropriated directory */
    chdir("/");

    /* Close all open file descriptors */
    int x;
    for (x = sysconf(_SC_OPEN_MAX); x>=0; x--)
    {
        close (x);
    }

    /* Open the log file */
    //openlog ("keepaliveaudio", LOG_PID, LOG_DAEMON);
}

void create_process()
{
    int exit_code;
    if(fork() == 0)
    {
        //OpenSLWrap_Init();
        printf("exec start\n");
        char* argv[] = {"./keepaliveaudio", "1", "&", 0};
        execv("./keepaliveaudio", argv);
        printf("exec done\n");
    }
    else
    {
        wait(&exit_code);
        if(WIFEXITED(exit_code))
        {
            /* Program terminated with exit */
            /* If you want, you could decode exit code here using
               WEXITSTATUS and you can start program again.
            */
            return;
        }
        else
        {
            /* Program didn't terminated with exit, restart */
            create_process();
        }
    }
}

int main(int argc, char* argv[])
{
    
    for (unsigned i =0; i < BUFFER_SIZE; i++) {
      buffer[2][i] = 0;//32768 - ((i % 100) * 660);
      buffer[1][i] = 0;//32768 - ((i % 100) * 660);
    }
    
    //printf("argc == %d\n", argc);
    
    //skeleton_daemon();
    //if (argc > 1) {
        printf("Running...\n", argc);
        OpenSLWrap_Init();
        while(1) sleep(1);
    //    return 0;
    //}
    
    //printf("Forking...\n", argc);
    //create_process();
    
    /*while (1)
    {
        //TODO: Insert daemon code here.
        //syslog (LOG_NOTICE, "keepaliveaudio started.");
        OpenSLWrap_Init();
        printf("OK\n");
        while(1) sleep(1);
        sleep (20);
        break;
    }*/

    //syslog (LOG_NOTICE, "keepaliveaudio started terminated.");
    //closelog();
    
    return 0;
}