#include <QDebug>
#include "workoutbot.h"
#include "parser/parser.h"
#include "db/dbhandler.h"

WorkoutBot::WorkoutBot(const std::string &token) : bot(token),
    waitForExerciseName(false),
    waitForSet(false),
    editModeOn(false)
{
//    start();
    setupCommands();
    setupCallbacks();
    setupMessages();
}

void WorkoutBot::start()
{
    /* Запуск */
    try {
        printf("Bot username: %s\n", bot.getApi().getMe()->username.c_str());
        TgBot::TgLongPoll longPoll(bot);
        while (true) {
            printf("Long poll started\n");
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
                                                    " Для того, чтобы начать, отправь мне команду  /run");
    });

    bot.getEvents().onCommand("run", [&](TgBot::Message::Ptr message) {
        std::shared_ptr<TgBot::InlineKeyboardButton> btnStart = std::make_shared<TgBot::InlineKeyboardButton>();
        btnStart->text = "Начать тренировку";
        btnStart->callbackData = "start_training";

        std::shared_ptr<TgBot::InlineKeyboardButton> getChartBtn = std::make_shared<TgBot::InlineKeyboardButton>();
        getChartBtn->text = "Получить отчет в виде графика";
        getChartBtn->callbackData = "get_chart";

        std::shared_ptr<TgBot::InlineKeyboardButton> getTextReportBtn = std::make_shared<TgBot::InlineKeyboardButton>();
        getTextReportBtn->text = "Получить отчет в виде текста";
        getTextReportBtn->callbackData = "get_text_report";

        TgBot::InlineKeyboardMarkup::Ptr keyboard = std::make_shared<TgBot::InlineKeyboardMarkup>();
        keyboard->inlineKeyboard.push_back({btnStart, getChartBtn, getTextReportBtn});

        bot.getApi().sendMessage(message->chat->id, "Выберите действие:", nullptr, 0, keyboard);
    });
}

void WorkoutBot::setupCallbacks()
{
    bot.getEvents().onCallbackQuery([&](TgBot::CallbackQuery::Ptr query) {
        if (query->data == "start_training") {
            std::shared_ptr<TgBot::KeyboardButton> addExerciseBtn = std::make_shared<TgBot::KeyboardButton>();
            addExerciseBtn->text = "Добавить упражнение";

            std::shared_ptr<TgBot::KeyboardButton> addSetBtn = std::make_shared<TgBot::KeyboardButton>();
            addSetBtn->text = "Добавить подход";

            std::shared_ptr<TgBot::KeyboardButton> editBtn = std::make_shared<TgBot::KeyboardButton>();
            editBtn->text = "Отредактировать вручуную";

            std::shared_ptr<TgBot::KeyboardButton> finishBtn = std::make_shared<TgBot::KeyboardButton>();
            finishBtn->text = "Завершить тренировку";

            std::shared_ptr<TgBot::ReplyKeyboardMarkup> _keyboard = std::make_shared<TgBot::ReplyKeyboardMarkup>();
            _keyboard->keyboard.push_back({addExerciseBtn, addSetBtn, editBtn, finishBtn});

            bot.getApi().sendMessage(query->message->chat->id, "Хорошей тренировки!", nullptr, 0, _keyboard);
        } else if (query->data == "get_chart"){
            /// Вз-е с классом Chart
        } else if (query->data == "finish_train"){
            /// пара<словарь<ключ: упражнение, значение: подходы>, дата_тренировки>
            QPair<QMap<QString, QList<double>>, QString> trainInfoAndDate = Parser::parseWorkoutMessage(currentTrainData);
            qDebug() << "Дата тренировки:" << trainInfoAndDate.second;
            for (auto it = trainInfoAndDate.first.constBegin(); it != trainInfoAndDate.first.constEnd(); ++it) {
                qDebug() << it.key() << ":" << it.value();
            }

//            DbHandler* instance = DbHandler::getInstance();
            /// Добавить какую то валидацию в этот метод
            DbHandler::getInstance()->saveTrain(trainInfoAndDate.second, trainInfoAndDate.first);
        } else if (query->data == "get_text_report") {
            QString dataStr;

            QMap<QString, double> data = DbHandler::getInstance()->getTxtReport();
            if (data.size() == 0) {
                bot.getApi().sendMessage(query->message->chat->id, "Ошибка, нет данных");
                return;
            }

            for (auto it = data.constBegin(); it != data.constEnd(); ++it){
                dataStr += it.key() + ": " + QString::number(it.value()) + "\n";
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
            std::shared_ptr<TgBot::InlineKeyboardButton> btnYes = std::make_shared<TgBot::InlineKeyboardButton>();
            btnYes->text = "Да";
            btnYes->callbackData = "finish_train";

            std::shared_ptr<TgBot::InlineKeyboardButton> btnNo = std::make_shared<TgBot::InlineKeyboardButton>();
            btnNo->text = "Нет";
            btnNo->callbackData = "cancel_action";

            TgBot::InlineKeyboardMarkup::Ptr keyboard = std::make_shared<TgBot::InlineKeyboardMarkup>();
            keyboard->inlineKeyboard.push_back({btnYes, btnNo});

            bot.getApi().sendMessage(message->chat->id, "Выберите действие:", nullptr, 0, keyboard);
        } else if (message->text == "Отредактировать вручуную"){
            bot.getApi().sendMessage(message->chat->id, "Скопируйте текст, отредактируйте и отправьте сообщением");
            editModeOn = true;

        } else if (editModeOn){
            currentTrainData = QString::fromStdString(message->text);
            editModeOn = false;
            qDebug() << currentTrainData;
        } else if (message->text == "Добавить упражнение" && !waitForExerciseName){
            bot.getApi().sendMessage(message->chat->id, "Введите название упражнения");
            waitForExerciseName = true;
            waitForSet = false;
        } else if (waitForExerciseName){
            /// Если это первое упражнение
            if (currentTrainData.isEmpty()) {
                Parser::init(currentTrainData, QString::fromStdString(message->text));
                bot.getApi().sendMessage(message->chat->id, "После ввода упражнения, нажмите кнопку добавить подход");
            } else {
                currentTrainData += QString::fromStdString(message->text) + "\n";
            }

            bot.getApi().sendMessage(message->chat->id, currentTrainData.toStdString());
            waitForExerciseName = false;
        } else if (message->text == "Добавить подход" && !waitForSet){
            bot.getApi().sendMessage(message->chat->id, "Введите подход в формате вес * количество повторений."
                                                        " К примеру 75*10 и отправьте как сообщение");
            waitForSet = true;
        } else if (waitForSet) {
            /// Вызвать static метод парсера
            QString setInfo = Parser::calcSetTonnage(QString::fromStdString(message->text));
            if (setInfo == "Ошибка! Неверный формат строки"){
                bot.getApi().sendMessage(message->chat->id, "Ошибка! Неверный формат строки");
            } else if (setInfo == "Ошибка! Не удалось выполнить преобразование, проверьте введенный формат") {
                bot.getApi().sendMessage(message->chat->id, "Ошибка! Не удалось выполнить преобразование, проверьте введенный формат");
            } else {
                currentTrainData += setInfo + "\n";
                bot.getApi().sendMessage(message->chat->id, currentTrainData.toStdString());
            }
        }
    });
}
