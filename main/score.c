#include "esp_log.h"
#include "esp_timer.h"
#include <time.h>

#include "score.h"

#define DEBUG               true

#define MAX_SCORE           10000
#define MAX_STEPS_WALKED    7000
#define MAX_STEPS_RAN       3500
#define MAX_WEAK_DUR        7200.0f // 120 minutes [s]
#define MAX_STRONG_DUR      1800.0f // 30 minutes [s]
#if DEBUG
    #define MULTIPLIER          100.0f
#else
    #define MULTIPLIER          1.0f
#endif

#define SAD                 0
#define NEUTRAL             2500
#define HAPPY               5000
#define EXCITED             9000

#if DEBUG
    #define DAY             false 
    // 1 day = 30 seconds
#else
    #define DAY             true
    // 1 day = 1 day
#endif

int steps_walked = 0;
int steps_ran = 0;

time_t current_day;

static const char *TAG = "SCORE";

time_t noramliseTime(time_t a_time) {
    struct tm *local = localtime(&a_time);

    if (DAY) {    
        local->tm_hour = 0;
        local->tm_min = 0;
        local->tm_sec = 0;
    } else {
        local->tm_sec = (local->tm_sec < 30) ? 0 : 30;
    }

    return mktime(local);
}

// Recalculate score with the new state of movement data. The score is calculated in such a manner, that 10,000 points can be reached max.
// This equals 7,000 walking steps, 3,500 running steps, 120 minutes of weak non-walking exercise, or 30 minutes of strong non-walking exercise.
// The mood of Move Mate is updated accordingly. The mood can only improve in a day, not worsen.
// The breakpoints for each mood are at 0 (sad), 2,500 (neutral), 5,000 (happy), and 9,000 (excited)
void updateScore(
    int                 *score,
    int                 step_count,
    movement_state_t    state,
    float               weak_duration,
    float               strong_duration,
    movement_mood_t     *mood
) {
    if (current_day == (time_t) -1) {
        current_day = noramliseTime(time(NULL));
    }

    if (state == STATE_WALKING) {
        steps_walked += step_count - steps_walked - steps_ran;
    } else if (state == STATE_RUNNING) {
        steps_ran += step_count - steps_walked - steps_ran;
    }

    *score = (int) (((float) steps_walked / MAX_STEPS_WALKED + (float) steps_ran / MAX_STEPS_RAN + weak_duration / MAX_WEAK_DUR + strong_duration / MAX_STRONG_DUR) * MAX_SCORE * MULTIPLIER);

    if (*score >= EXCITED && *mood == MOOD_HAPPY) {
        *mood = MOOD_EXCITED;
        ESP_LOGI(TAG, "Change mood from HAPPY to EXCITED.");
    } else if (*score >= HAPPY && *mood == MOOD_NEUTRAL) {
        *mood = MOOD_HAPPY;
        ESP_LOGI(TAG, "Change mood from NEUTRAL to HAPPY.");
    } else if (*score >= NEUTRAL && *mood == MOOD_SAD) {
        *mood = MOOD_NEUTRAL;
        ESP_LOGI(TAG, "Change mood from SAD to NEUTRAL.");
    } else if (*score >= SAD && *mood == MOOD_DEAD) {
        *mood = MOOD_NEUTRAL;
        ESP_LOGI(TAG, "Change mood from DEAD to NEUTRAL.");
    }
}

// Checks if a new day has begun and sets the mood accordingly. The mood decreases only if the score and current mood do not align, which means that the mood
// worsens over time if there is no or insufficient movement. The deaths, streaks, and movement data are also getting (re-)set.
bool checkDayChange(
    int             *score,
    int             *deaths,
    int             *streak,
    movement_mood_t *mood
) {
    time_t the_time = time(NULL);

    if ((DAY && difftime(the_time, current_day) >= 86400) || difftime(the_time, current_day) >= 30) {
        the_time = noramliseTime(the_time);
        ESP_LOGI(TAG, "Previous day: %s", ctime(&current_day));
        ESP_LOGI(TAG, "New day: %s", ctime(&the_time));
        current_day = the_time;

        if (*mood == MOOD_DEAD && *score == 0) {
            *mood = MOOD_DEAD;
            *deaths = *deaths;
            *streak = 0;
        } else if (*mood == MOOD_SAD && *score < NEUTRAL) {
            *mood = MOOD_DEAD;
            *deaths = *deaths + 1;
            *streak = 0;
        } else if (*mood == MOOD_NEUTRAL && *score < NEUTRAL) {
            *mood = MOOD_SAD;
            *streak = *streak + 1;
        } else if (*mood == MOOD_HAPPY && *score < HAPPY) {
            *mood = MOOD_NEUTRAL;
            *streak = *streak + 1;
        } else if (*mood == MOOD_EXCITED && *score < EXCITED) {
            *mood = MOOD_HAPPY;
            *streak = *streak + 1;
        } else {
            *streak = *streak + 1;
        }

        ESP_LOGI(TAG, "Day change: deaths %d, streak %d", *deaths, *streak);

        *score = 0;
        steps_walked = 0;
        steps_ran = 0;
        return true;
    }

    return false;
}