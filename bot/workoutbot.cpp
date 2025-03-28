#include <QDebug>
#include <QScopedPointer>

#include "workoutbot.h"
#include "parser/parser.h"
#include "db/dbhandler.h"
#include "chart/chart.h"

WorkoutBot::WorkoutBot(const std::string &token) : bot(token),
    waitForExerciseName(false),
    waitForSet(false),
    editModeOn(false)
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
        if (query->data == "start_training") {
            bot.getApi().sendMessage(query->message->chat->id, "Добавьте упражнение", nullptr, 0, _keyboard);
        } else if (query->data == "get_chart"){
            QMap<QString, double> data = DbHandler::getInstance()->trainData(query->message->chat->id);
            if (data.size() == 0) {
                bot.getApi().sendMessage(query->message->chat->id, "Ошибка, нет данных для построения графика");
                return;
            }

            QScopedPointer<Chart>_chart(new Chart(query->message->chat->id));
            _chart->createPlot();
            /// Для каждого пользователя будет храниться по одной картинке в директории charts, название файла -> id чата
            QString path = "charts/" + QString::number(query->message->chat->id) + ".png";
            bot.getApi().sendPhoto(query->message->chat->id, TgBot::InputFile::fromFile(path.toStdString(), "image/png"));
        } else if (query->data == "finish_train"){
            if (!usersTrainData.contains(query->message->chat->id) || usersTrainData.value(query->message->chat->id).isEmpty()) {
                bot.getApi().sendMessage(query->message->chat->id, "Ошибка, тренировка не может быть пустой");
                return;
            }
            /// пара<словарь<ключ: упражнение, значение: подходы>, дата_тренировки>
            QPair<QMap<QString, QList<double>>, QString> trainInfoAndDate = Parser::parseWorkoutMessage(usersTrainData.value(query->message->chat->id));
            bool ok = DbHandler::getInstance()->saveTrain(query->message->chat->id, trainInfoAndDate.second, trainInfoAndDate.first);
            if (ok) {
                /// После успешного сохранения, сбрасываем маркеры, позволяющие отслеживать состояние бота
                waitForExerciseName = false;
                waitForSet = false;
                editModeOn = false;
                usersTrainData.remove(query->message->chat->id);
                bot.getApi().sendMessage(query->message->chat->id, "Тренировка успешно сохранена");
            } else {
                bot.getApi().sendMessage(query->message->chat->id, "Ошибка, попробуйте еще раз");
            }

        } else if (query->data == "get_text_report") {
            QString dataStr;

            QMap<QString, double> data = DbHandler::getInstance()->trainData(query->message->chat->id);
            if (data.size() == 0) {
                bot.getApi().sendMessage(query->message->chat->id, "Ошибка, нет данных");
                return;
            }

            for (auto it = data.constBegin(); it != data.constEnd(); ++it){
                dataStr += it.key() + ": " + QString::number(it.value()) + "кг" +  "\n";
            }

            bot.getApi().sendMessage(query->message->chat->id, dataStr.toStdString());
        }

        bot.getApi().answerCallbackQuery(query->id);
    });

}

void WorkoutBot::setupMessages()
{
    bot.getEvents().onAnyMessage([&](TgBot::Message::Ptr message){
        if (message->text == "Завершить тренировку"){
            bot.getApi().sendMessage(message->chat->id, "Завершить тренировку?");
            bot.getApi().sendMessage(message->chat->id, "Выберите действие:", nullptr, 0, answerKeyboard);
        } else if (message->text == "Отредактировать вручную"){
            bot.getApi().sendMessage(message->chat->id, "Скопируйте текст, отредактируйте и отправьте сообщением");
            editModeOn = true;
        } else if (editModeOn){
            usersTrainData[message->chat->id] = QString::fromStdString(message->text + "\n");
            editModeOn = false;
            bot.getApi().sendMessage(message->chat->id, "Текст успешно отформатирован");
        } else if (message->text == "+ упражнение" && !waitForExerciseName){
            bot.getApi().sendMessage(message->chat->id, "Введите название упражнения");
            waitForExerciseName = true;
            waitForSet = false;
        } else if (waitForExerciseName){
            /// Если это первое упражнение
            if (!usersTrainData.contains(message->chat->id)) {
                Parser::init(usersTrainData[message->chat->id], QString::fromStdString(message->text));
            } else {
                /// приводим первую букву упражнения к верхнему регистру
                QString origStr = QString::fromStdString(message->text);
                origStr[0] = origStr[0].toUpper();
                usersTrainData[message->chat->id] += origStr + "\n";
            }

            bot.getApi().sendMessage(message->chat->id, usersTrainData.value(message->chat->id).toStdString());
            bot.getApi().sendMessage(message->chat->id, "После ввода названи упражнения не забудьте нажать +подход");
            waitForExerciseName = false;
        } else if (message->text == "+ подход" && !waitForSet){
            bot.getApi().sendMessage(message->chat->id, "Введите подход в формате вес * количество повторений."
                                                        " К примеру 75*10 и отправьте как сообщение");
            waitForSet = true;
        } else if (message->text == "+ подход" && waitForSet){
            bot.getApi().sendMessage(message->chat->id, "Итак жду подхода");
        } else if (waitForSet && message->text != "меню") {
            QString setInfo = Parser::calcSetTonnage(QString::fromStdString(message->text));
            if (setInfo == "Ошибка! Неверный формат строки"){
                bot.getApi().sendMessage(message->chat->id, "Ошибка! Неверный формат строки");
            } else if (setInfo == "Ошибка! Не удалось выполнить преобразование, проверьте введенный формат") {
                bot.getApi().sendMessage(message->chat->id, "Ошибка! Не удалось выполнить преобразование, проверьте введенный формат");
            } else {
                usersTrainData[message->chat->id] += setInfo + "\n";
                bot.getApi().sendMessage(message->chat->id, usersTrainData.value(message->chat->id).toStdString());
            }
        } else if (message->text == "меню"){
            bot.getApi().sendMessage(message->chat->id, "Выберите действие:", nullptr, 0, _inlineKeyboard);
        }
    });
}
