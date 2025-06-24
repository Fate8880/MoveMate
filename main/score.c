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

time_t start_time;
time_t current_day;

static const char *TAG = "SCORE";

void update_score(
    int                 *score,
    int                 step_count,
    movement_state_t    state,
    float               weak_duration,
    float               strong_duration,
    movement_mood_t     *mood
) {
    if (start_time == (time_t) -1) {
        start_time = time(NULL);
        current_day = start_time;
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

bool checkDayChange(
    int             *score,
    int             *deaths,
    int             *streak,
    movement_mood_t *mood
) {
    time_t the_time = time(NULL);

    if ((DAY && difftime(the_time, current_day) >= 86400) || difftime(the_time, current_day) >= 30) {
        current_day = the_time;

        if (*mood == MOOD_SAD && *score < NEUTRAL) {
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
        }

        ESP_LOGI(TAG, "Day change: deaths %d, streak %d", *deaths, *streak);

        *score = 0;
        steps_walked = 0;
        steps_ran = 0;
        return true;
    }

    return false;
}