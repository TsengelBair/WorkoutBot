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
                                                    " Для того, чтобы начать, отправь мне команду /run");
    });

    bot.getEvents().onCommand("run", [&](TgBot::Message::Ptr message) {
        std::shared_ptr<TgBot::InlineKeyboardButton> btnStart = std::make_shared<TgBot::InlineKeyboardButton>();
        btnStart->text = "Тренировка";
        btnStart->callbackData = "start_training";

        std::shared_ptr<TgBot::InlineKeyboardButton> getChartBtn = std::make_shared<TgBot::InlineKeyboardButton>();
        getChartBtn->text = "График";
        getChartBtn->callbackData = "get_chart";

        std::shared_ptr<TgBot::InlineKeyboardButton> getTextReportBtn = std::make_shared<TgBot::InlineKeyboardButton>();
        getTextReportBtn->text = "Отчет";
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
            addExerciseBtn->text = "+ упражнение";

            std::shared_ptr<TgBot::KeyboardButton> addSetBtn = std::make_shared<TgBot::KeyboardButton>();
            addSetBtn->text = "+ подход";

            std::shared_ptr<TgBot::KeyboardButton> editBtn = std::make_shared<TgBot::KeyboardButton>();
            editBtn->text = "Отредактировать вручную";

            std::shared_ptr<TgBot::KeyboardButton> finishBtn = std::make_shared<TgBot::KeyboardButton>();
            finishBtn->text = "Завершить тренировку";

            std::shared_ptr<TgBot::KeyboardButton> menuBtn = std::make_shared<TgBot::KeyboardButton>();
            menuBtn->text = "меню";

            std::shared_ptr<TgBot::ReplyKeyboardMarkup> _keyboard = std::make_shared<TgBot::ReplyKeyboardMarkup>();
            _keyboard->keyboard.push_back({addExerciseBtn, addSetBtn, editBtn});
            _keyboard->keyboard.push_back({finishBtn, menuBtn});

            bot.getApi().sendMessage(query->message->chat->id, "Добавьте упражнение", nullptr, 0, _keyboard);
        } else if (query->data == "get_chart"){
            QScopedPointer<Chart>_chart(new Chart(query->message->chat->id));
            _chart->createPlot();
            /// Для каждого пользователя будет храниться по одной картинке в текущей директории, файл будет называться также как и id чата
            QString path = QString::number(query->message->chat->id) + ".png";
            qDebug() << "Path to file:" << path;
            bot.getApi().sendPhoto(query->message->chat->id, TgBot::InputFile::fromFile(path.toStdString(), "image/png"));
        } else if (query->data == "finish_train"){
            /// пара<словарь<ключ: упражнение, значение: подходы>, дата_тренировки>
            QPair<QMap<QString, QList<double>>, QString> trainInfoAndDate = Parser::parseWorkoutMessage(currentTrainData);
            qDebug() << "Дата тренировки:" << trainInfoAndDate.second;
            for (auto it = trainInfoAndDate.first.constBegin(); it != trainInfoAndDate.first.constEnd(); ++it) {
                qDebug() << it.key() << ":" << it.value();
            }
            /// Добавить какую то валидацию в этот метод
            bool ok = DbHandler::getInstance()->saveTrain(trainInfoAndDate.second, trainInfoAndDate.first);
            if (ok) {
                waitForExerciseName = false;
                waitForSet = false;
                editModeOn = false;
                bot.getApi().sendMessage(query->message->chat->id, "Тренировка успешно сохранена");
            } else {
                bot.getApi().sendMessage(query->message->chat->id, "Ошибка, попробуйте еще раз");
            }

        } else if (query->data == "get_text_report") {
            QString dataStr;

            QMap<QString, double> data = DbHandler::getInstance()->trainData();
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
            std::shared_ptr<TgBot::InlineKeyboardButton> btnYes = std::make_shared<TgBot::InlineKeyboardButton>();
            btnYes->text = "Да";
            btnYes->callbackData = "finish_train";

            std::shared_ptr<TgBot::InlineKeyboardButton> btnNo = std::make_shared<TgBot::InlineKeyboardButton>();
            btnNo->text = "Нет";
            btnNo->callbackData = "cancel_action";

            TgBot::InlineKeyboardMarkup::Ptr keyboard = std::make_shared<TgBot::InlineKeyboardMarkup>();
            keyboard->inlineKeyboard.push_back({btnYes, btnNo});

            bot.getApi().sendMessage(message->chat->id, "Выберите действие:", nullptr, 0, keyboard);
        } else if (message->text == "Отредактировать вручную"){
            bot.getApi().sendMessage(message->chat->id, "Скопируйте текст, отредактируйте и отправьте сообщением");
            editModeOn = true;

        } else if (editModeOn){
            currentTrainData = QString::fromStdString(message->text + "\n");
            editModeOn = false;
            bot.getApi().sendMessage(message->chat->id, "Текст успешно отформатирован");
        } else if (message->text == "+ упражнение" && !waitForExerciseName){
            bot.getApi().sendMessage(message->chat->id, "Введите название упражнения");
            waitForExerciseName = true;
            waitForSet = false;
        } else if (waitForExerciseName){
            /// Если это первое упражнение
            if (currentTrainData.isEmpty()) {
                Parser::init(currentTrainData, QString::fromStdString(message->text));
            } else {
                currentTrainData += QString::fromStdString(message->text) + "\n";
            }

            bot.getApi().sendMessage(message->chat->id, currentTrainData.toStdString());
            bot.getApi().sendMessage(message->chat->id, "После ввода названи упражнения не забудьте нажать +подход");
            waitForExerciseName = false;
        } else if (message->text == "+ подход" && !waitForSet){
            bot.getApi().sendMessage(message->chat->id, "Введите подход в формате вес * количество повторений."
                                                        " К примеру 75*10 и отправьте как сообщение");
            waitForSet = true;
        } else if (message->text == "+ подход" && waitForSet){
            bot.getApi().sendMessage(message->chat->id, "Итак жду подхода");
        } else if (waitForSet && message->text != "меню") {
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
        } else if (message->text == "меню"){
            /// Дублирую код (потом исправить)
            std::shared_ptr<TgBot::InlineKeyboardButton> btnStart = std::make_shared<TgBot::InlineKeyboardButton>();
            btnStart->text = "Тренировка";
            btnStart->callbackData = "start_training";

            std::shared_ptr<TgBot::InlineKeyboardButton> getChartBtn = std::make_shared<TgBot::InlineKeyboardButton>();
            getChartBtn->text = "График";
            getChartBtn->callbackData = "get_chart";

            std::shared_ptr<TgBot::InlineKeyboardButton> getTextReportBtn = std::make_shared<TgBot::InlineKeyboardButton>();
            getTextReportBtn->text = "Отчет";
            getTextReportBtn->callbackData = "get_text_report";

            TgBot::InlineKeyboardMarkup::Ptr keyboard = std::make_shared<TgBot::InlineKeyboardMarkup>();
            keyboard->inlineKeyboard.push_back({btnStart, getChartBtn, getTextReportBtn});

            bot.getApi().sendMessage(message->chat->id, "Выберите действие:", nullptr, 0, keyboard);
        }
    });
}
