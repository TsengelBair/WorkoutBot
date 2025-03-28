#include <QDebug>
#include <QScopedPointer>

#include "workoutbot.h"
#include "parser/parser.h"
#include "db/dbhandler.h"
#include "chart/chart.h"

WorkoutBot::WorkoutBot(const std::string &token) : bot(token)
{
    /// кнопки для _inlineKeyboard
    btnStart = std::make_shared<TgBot::InlineKeyboardButton>();
    btnStart->text = "Тренировка";
    btnStart->callbackData = "start_training";

    getChartBtn = std::make_shared<TgBot::InlineKeyboardButton>();
    getChartBtn->text = "График";
    getChartBtn->callbackData = "get_chart";

    getTextReportBtn = std::make_shared<TgBot::InlineKeyboardButton>();
    getTextReportBtn->text = "Отчет";
    getTextReportBtn->callbackData = "get_text_report";

    _inlineKeyboard = std::make_shared<TgBot::InlineKeyboardMarkup>();
    _inlineKeyboard->inlineKeyboard.push_back({btnStart, getChartBtn, getTextReportBtn});

    ///кнопки для _keyboard
    addExerciseBtn = std::make_shared<TgBot::KeyboardButton>();
    addExerciseBtn->text = "+ упражнение";

    addSetBtn = std::make_shared<TgBot::KeyboardButton>();
    addSetBtn->text = "+ подход";

    editBtn = std::make_shared<TgBot::KeyboardButton>();
    editBtn->text = "Отредактировать вручную";

    finishBtn = std::make_shared<TgBot::KeyboardButton>();
    finishBtn->text = "Завершить тренировку";

    menuBtn = std::make_shared<TgBot::KeyboardButton>();
    menuBtn->text = "меню";

    _keyboard = std::make_shared<TgBot::ReplyKeyboardMarkup>();
    _keyboard->keyboard.push_back({addExerciseBtn, addSetBtn, editBtn});
    _keyboard->keyboard.push_back({finishBtn, menuBtn});

    btnYes = std::make_shared<TgBot::InlineKeyboardButton>();
    btnYes->text = "Да";
    btnYes->callbackData = "finish_train";

    btnNo = std::make_shared<TgBot::InlineKeyboardButton>();
    btnNo->text = "Нет";
    btnNo->callbackData = "cancel_action";

    answerKeyboard = std::make_shared<TgBot::InlineKeyboardMarkup>();
    answerKeyboard->inlineKeyboard.push_back({btnYes, btnNo});

    setupCommands();
    setupCallbacks();
    setupMessages();
}

void WorkoutBot::start()
{
    /// Запуск
    try {
        TgBot::TgLongPoll longPoll(bot);
        while (true) {
            longPoll.start();
        }
    } catch (TgBot::TgException& e) {
        printf("error: %s\n", e.what());
    }
}
void WorkoutBot::setupCommands()
{
    bot.getEvents().onCommand("start", [&](TgBot::Message::Ptr message) {
        bot.getApi().sendMessage(message->chat->id, "Привет! Я помогу тебе отслеживать свой прогресс!"
                                                    " Для того, чтобы начать, отправь мне команду /run");
    });

    bot.getEvents().onCommand("run", [&](TgBot::Message::Ptr message) {
        bot.getApi().sendMessage(message->chat->id, "Выберите действие:", nullptr, 0, _inlineKeyboard);
    });
}

void WorkoutBot::setupCallbacks()
{
    bot.getEvents().onCallbackQuery([&](TgBot::CallbackQuery::Ptr query) {

        std::int64_t chatId = query->message->chat->id;
        if (!userStates.contains(chatId)) {
            userStates[chatId] = UserState{false, false, false, ""};
        }

        UserState& state = userStates[chatId];

        if (query->data == "start_training") {
            bot.getApi().sendMessage(chatId, "Добавьте упражнение", nullptr, 0, _keyboard);
        } else if (query->data == "get_chart"){
            QMap<QString, double> data = DbHandler::getInstance()->trainData(chatId);
            if (data.size() == 0) {
                bot.getApi().sendMessage(chatId, "Ошибка, нет данных для построения графика");
                return;
            }

            QScopedPointer<Chart>_chart(new Chart(chatId));
            _chart->createPlot();
            /// Для каждого пользователя будет храниться по одной картинке в директории charts, название файла -> id чата
            QString path = "charts/" + QString::number(chatId) + ".png";
            bot.getApi().sendPhoto(chatId, TgBot::InputFile::fromFile(path.toStdString(), "image/png"));
        } else if (query->data == "finish_train"){
            if (state.usersTrainDataStr.isEmpty()) {
                bot.getApi().sendMessage(chatId, "Ошибка, тренировка не может быть пустой");
                return;
            }
            /// пара<словарь<ключ: упражнение, значение: подходы>, дата_тренировки>
            QPair<QMap<QString, QList<double>>, QString> trainInfoAndDate = Parser::parseWorkoutMessage(state.usersTrainDataStr);
            QString error;
            bool ok = DbHandler::getInstance()->saveTrain(chatId, trainInfoAndDate.second,
                                                          trainInfoAndDate.first, error);
            if (ok) {
                /// После успешного сохранения, сбрасываем маркеры, позволяющие отслеживать состояние бота
                state = UserState{false, false, false, ""};
                bot.getApi().sendMessage(chatId, "Тренировка успешно сохранена");
            } else {
                if (error.contains("UNIQUE constraint failed")) {
                    bot.getApi().sendMessage(chatId, "Ошибка, за день может быть добавлена только одна тренировка");
                } else {
                    bot.getApi().sendMessage(chatId, "Ошибка, попробуйте еще раз");
                }
            }

        } else if (query->data == "get_text_report") {
            QString dataStr;

            QMap<QString, double> data = DbHandler::getInstance()->trainData(chatId);
            if (data.size() == 0) {
                bot.getApi().sendMessage(chatId, "Ошибка, нет данных");
                return;
            }

            for (auto it = data.constBegin(); it != data.constEnd(); ++it){
                dataStr += it.key() + ": " + QString::number(it.value()) + "кг" +  "\n";
            }

            bot.getApi().sendMessage(chatId, dataStr.toStdString());
        }

        bot.getApi().answerCallbackQuery(query->id);
    });

}

void WorkoutBot::setupMessages()
{
    bot.getEvents().onAnyMessage([&](TgBot::Message::Ptr message){

        std::int64_t chatId = message->chat->id;
        if (!userStates.contains(chatId)) {
            userStates[chatId] = UserState{false, false, false, ""};
        }

        UserState& state = userStates[chatId];

        if (message->text == "Завершить тренировку"){
            bot.getApi().sendMessage(chatId, "Завершить тренировку?");
            bot.getApi().sendMessage(chatId, "Выберите действие:", nullptr, 0, answerKeyboard);
        } else if (message->text == "Отредактировать вручную"){
            bot.getApi().sendMessage(chatId, "Скопируйте текст, отредактируйте и отправьте сообщением");
            state.editModeOn = true;
        } else if (state.editModeOn){
            state.usersTrainDataStr = QString::fromStdString(message->text + "\n");
            state.editModeOn = false;
            bot.getApi().sendMessage(chatId, "Текст успешно отформатирован");
        } else if (message->text == "+ упражнение" && !state.waitForExerciseName){
            bot.getApi().sendMessage(chatId, "Введите название упражнения");
            state.waitForExerciseName = true;
            state.waitForSet = false;
        } else if (state.waitForExerciseName){
            if (message->text == "+ подход") {
                bot.getApi().sendMessage(chatId, "Вы не ввели название упражнения");
                return;
            } else if (message->text == "+ упражнение") {
                bot.getApi().sendMessage(chatId, "Жду название упражнения");
                return;
            }

            /// Если это первое упражнение
            if (state.usersTrainDataStr.isEmpty()) {
                Parser::init(state.usersTrainDataStr, QString::fromStdString(message->text));
            } else {
                /// приводим первую букву упражнения к верхнему регистру
                QString origStr = QString::fromStdString(message->text);
                origStr[0] = origStr[0].toUpper();
                state.usersTrainDataStr += origStr + "\n";
            }

            bot.getApi().sendMessage(chatId, state.usersTrainDataStr.toStdString());
            bot.getApi().sendMessage(chatId, "После ввода названия упражнения не забудьте нажать +подход");
            state.waitForExerciseName = false;
        } else if (message->text == "+ подход" && !state.waitForSet){
            bot.getApi().sendMessage(chatId, "Введите подход в формате вес * количество повторений."
                                                        " К примеру 75*10 и отправьте как сообщение");
            state.waitForSet = true;
        } else if (message->text == "+ подход" && state.waitForSet){
            bot.getApi().sendMessage(chatId, "Итак жду подхода");
        } else if (state.waitForSet && message->text != "меню") {
            QString setInfo = Parser::calcSetTonnage(QString::fromStdString(message->text));
            if (setInfo == "Ошибка! Неверный формат строки"){
                bot.getApi().sendMessage(chatId, "Ошибка! Неверный формат строки");
            } else if (setInfo == "Ошибка! Не удалось выполнить преобразование, проверьте введенный формат") {
                bot.getApi().sendMessage(chatId, "Ошибка! Не удалось выполнить преобразование, проверьте введенный формат");
            } else {
                state.usersTrainDataStr += setInfo + "\n";
                bot.getApi().sendMessage(chatId, state.usersTrainDataStr.toStdString());
            }
        } else if (message->text == "меню"){
            bot.getApi().sendMessage(message->chat->id, "Выберите действие:", nullptr, 0, _inlineKeyboard);
        }
    });
}
