// written by diskodev
// engine/fmv.c
#include "fmv.h"

void FMV_Init(FMVPlayer* fmv, cstr videoPath, bool loop) {
    if (!fmv || !pd) return;
    
    fmv->videoPath = videoPath;
    fmv->isLooping = loop;
    fmv->currentFrame = 0;
    fmv->frameAccumulator = 0.0f;
    fmv->isPlaying = false;
    
    // Load the video file
    fmv->player = pd->graphics->video->loadVideo(videoPath);
    
    if (!fmv->player) {
        cstr error = pd->graphics->video->getError(fmv->player);
        pd->system->logToConsole("Failed to load video '%s': %s", videoPath, error ? error : "Unknown error");
        return;
    }
    
    // Get video information
    pd->graphics->video->getInfo(fmv->player, &fmv->width, &fmv->height, 
                                  &fmv->framerate, &fmv->totalFrames, NULL);
    
    // Use the screen as the drawing context
    pd->graphics->video->useScreenContext(fmv->player);
    
    pd->system->logToConsole("FMV loaded: %dx%d @ %.2f fps, %d frames", 
                            fmv->width, fmv->height, (double)fmv->framerate, fmv->totalFrames);
}

void FMV_Update(FMVPlayer* fmv, f32 deltaTime) {
    if (!fmv || !fmv->player || !fmv->isPlaying) return;
    
    // Accumulate time to advance frames
    fmv->frameAccumulator += deltaTime * fmv->framerate;
    
    // Advance frames based on accumulated time
    while (fmv->frameAccumulator >= 1.0f) {
        fmv->currentFrame++;
        fmv->frameAccumulator -= 1.0f;
        
        // Check if we've reached the end
        if (fmv->currentFrame >= fmv->totalFrames) {
            if (fmv->isLooping) {
                fmv->currentFrame = 0;
            } else {
                fmv->currentFrame = fmv->totalFrames - 1;
                fmv->isPlaying = false;
            }
        }
    }
}

void FMV_Draw(FMVPlayer* fmv) {
    if (!fmv || !fmv->player) return;
    
    // Render the current frame to the screen context
    pd->graphics->video->renderFrame(fmv->player, fmv->currentFrame);
}

void FMV_Free(FMVPlayer* fmv) {
    if (!fmv) return;
    
    if (fmv->player) {
        pd->graphics->video->freePlayer(fmv->player);
        fmv->player = NULL;
    }
    
    fmv->isPlaying = false;
    fmv->currentFrame = 0;
}

void FMV_Play(FMVPlayer* fmv) {
    if (!fmv || !fmv->player) return;
    fmv->isPlaying = true;
}

void FMV_Pause(FMVPlayer* fmv) {
    if (!fmv) return;
    fmv->isPlaying = false;
}

void FMV_Stop(FMVPlayer* fmv) {
    if (!fmv) return;
    fmv->isPlaying = false;
    fmv->currentFrame = 0;
    fmv->frameAccumulator = 0.0f;
}

void FMV_Seek(FMVPlayer* fmv, int frame) {
    if (!fmv || !fmv->player) return;
    
    if (frame < 0) frame = 0;
    if (frame >= fmv->totalFrames) frame = fmv->totalFrames - 1;
    
    fmv->currentFrame = frame;
    fmv->frameAccumulator = 0.0f;
}

bool FMV_IsFinished(FMVPlayer* fmv) {
    if (!fmv || !fmv->player) return true;
    return !fmv->isPlaying && fmv->currentFrame >= fmv->totalFrames - 1;
}
