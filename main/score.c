#include "esp_log.h"
#include "esp_timer.h"

#include "score.h"

int prev_score = 0;
int prev_deaths = 0;
int prev_streak = 0;
float prev_weak_dur = .0f;
float prev_strong_dur = .0f;

static const char *TAG = "SCORE";

// TODO current day

void update_score(
    int             *score,
    int             *deaths,
    int             *streak,
    int             step_count,
    movement_mood_t *mood,
    float           weak_duration,
    float           strong_duration
) {
    ESP_LOGI(TAG, "Score %d, Streak %d, Deaths %d, Mood %d, Steps %d, Weak Duration %.2f, Strong Duration %.2f", *score, *streak, *deaths, (int) *mood, step_count, weak_duration, strong_duration);
}