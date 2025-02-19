#include <QCoreApplication>

#include <tgbot/tgbot.h>
#include "bot/workoutbot.h"
#include "config.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    WorkoutBot bot(_token);
    bot.start();

    return a.exec();
}
