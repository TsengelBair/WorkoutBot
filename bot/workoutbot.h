#ifndef WORKOUTBOT_H
#define WORKOUTBOT_H

#include <tgbot/tgbot.h>
#include <string>
#include <QString>

class WorkoutBot
{
public:
    explicit WorkoutBot(const std::string& token);
    void start();

private:
    void setupCommands();
    void setupCallbacks();
    void setupMessages();

private:
    TgBot::Bot bot;
    QString currentTrainData;

    bool waitForExerciseName;
    bool waitForSet;
    bool editModeOn;
};

#endif // WORKOUTBOT_H
