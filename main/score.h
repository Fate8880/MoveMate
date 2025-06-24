// score.h
#ifndef SCORE_H
#define SCORE_H

#include "display.h"

void update_score(
    int                 *score,
    int                 step_count,
    movement_state_t    state,
    float               weak_duration,
    float               strong_duration,
    movement_mood_t     *mood
);

bool checkDayChange(
    int             *score,
    int             *deaths,
    int             *streak,
    movement_mood_t *mood
);

#endif // SCORE_H
