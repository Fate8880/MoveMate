// score.h
#ifndef SCORE_H
#define SCORE_H

#include "display.h"

void update_score(
    int             *score,
    int             *deaths,
    int             *streak,
    int             step_count,
    movement_mood_t *mood,
    float           weak_duration,
    float           strong_duration
);

#endif // SCORE_H
