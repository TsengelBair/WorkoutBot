#ifndef WORKOUTBOT_H
#define WORKOUTBOT_H

#include <tgbot/tgbot.h>
#include <QHash>
#include <QString>

/// для отслеживания состояния бота (создается под каждого пользователя)
struct UserState {
    bool waitForExerciseName;
    bool waitForSet;
    bool editModeOn;
    QString usersTrainDataStr;
};

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
    std::shared_ptr<TgBot::InlineKeyboardButton> btnStart;
    std::shared_ptr<TgBot::InlineKeyboardButton> getChartBtn;
    std::shared_ptr<TgBot::InlineKeyboardButton> getTextReportBtn;
    std::shared_ptr<TgBot::InlineKeyboardButton> exercisesBtn;
    TgBot::InlineKeyboardMarkup::Ptr _inlineKeyboard;

    std::shared_ptr<TgBot::KeyboardButton> addExerciseBtn;
    std::shared_ptr<TgBot::KeyboardButton> addSetBtn;
    std::shared_ptr<TgBot::KeyboardButton> editBtn;
    std::shared_ptr<TgBot::KeyboardButton> finishBtn;
    std::shared_ptr<TgBot::KeyboardButton> menuBtn;
    std::shared_ptr<TgBot::ReplyKeyboardMarkup> _keyboard;

    std::shared_ptr<TgBot::InlineKeyboardButton> btnYes;
    std::shared_ptr<TgBot::InlineKeyboardButton> btnNo;
    TgBot::InlineKeyboardMarkup::Ptr answerKeyboard;

private:
    TgBot::Bot bot;
    QHash<std::int64_t, UserState>userStates;
};

#endif // WORKOUTBOT_H
