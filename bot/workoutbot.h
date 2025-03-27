#ifndef WORKOUTBOT_H
#define WORKOUTBOT_H

#include <tgbot/tgbot.h>
#include <QHash>
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

/// переиспользуемые виджеты
private:
    std::shared_ptr<TgBot::InlineKeyboardButton> btnStart;
    std::shared_ptr<TgBot::InlineKeyboardButton> getChartBtn;
    std::shared_ptr<TgBot::InlineKeyboardButton> getTextReportBtn;
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
    QString currentTrainData;

    QHash<std::int64_t, QString>usersTrainData;

    bool waitForExerciseName;
    bool waitForSet;
    bool editModeOn;
};

#endif // WORKOUTBOT_H
