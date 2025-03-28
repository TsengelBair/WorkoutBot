#include <QDateTime>
#include <QMap>
#include "parser.h"

Parser::Parser()
{
}

void Parser::init(QString &currentTrainData, QString firstExercise)
{
    QDateTime currentDateAndTime = QDateTime::currentDateTime();

    QString dateHeader = "Дата тренировки: " +  currentDateAndTime.date().toString("dd-MM-yyyy");
    QString timeHeader = "Начало тренировки: " + currentDateAndTime.time().toString("HH:mm");

    /// приводим название упражнения к верхнему регистру
    QString _firstExercise = firstExercise;
    _firstExercise[0] = _firstExercise[0].toUpper();

    currentTrainData +=  dateHeader + "\n";
    currentTrainData +=  timeHeader + "\n";
    currentTrainData +=  _firstExercise + "\n";
}

QString Parser::calcSetTonnage(const QString &input)
{
    /// убираем лишние пробелы
    QString cleanInput = input.simplified();
    /// запятые на точки
    cleanInput.replace(',', '.');

    QStringList parts = cleanInput.split("*");
    if (parts.size() != 2){
        return "Ошибка! Неверный формат строки";
    }

    bool ok1, ok2;
    double num1 = parts[0].toDouble(&ok1);
    double num2 = parts[1].toDouble(&ok2);

    if (!ok1 || !ok2) {
        return "Ошибка! Не удалось выполнить преобразование, проверьте введенный формат";
    }

    double res = num1 * num2;

    return QString("%1 * %2 = %3").arg(num1).arg(num2).arg(res);
}

QPair<QMap<QString, QList<double>>, QString> Parser::parseWorkoutMessage(const QString &message)
{
    QMap<QString, QList<double>> exercisesTonnage; /// Хранит тоннаж для каждого упражнения
    QString date;

    /// Разделяем сообщение на строки
    QStringList lines = message.split("\n", QString::SkipEmptyParts);

    /// Извлекаем дату из первой строки
    if (!lines.isEmpty() && lines[0].startsWith("Дата тренировки:")) {
        date = lines[0].section(':', 1).trimmed();
    }

    /// Начинаем с третьей строки (индекс 2), чтобы пропустить дату и время
    QString currentExercise;
    for (int i = 2; i < lines.size(); ++i) {
        const QString &line = lines[i];

        /// Если строка содержит только цифры, дробные числа и символы, это подход
        if (line.contains(QRegExp("^\\d+(\\.\\d+)?\\s*\\*\\s*\\d+(\\.\\d+)?\\s*=\\s*\\d+(\\.\\d+)?$"))) {
            if (!currentExercise.isEmpty()) {
                // Извлекаем тоннаж из строки
                QString tonnageStr = line.section("=", 1).trimmed();
                tonnageStr.replace(',', '.'); // Заменяем запятую на точку
                double tonnage = tonnageStr.toDouble();
                exercisesTonnage[currentExercise].append(tonnage);
            }
        } else {
            /// Это название нового упражнения, убираем лишние пробелы и приводим первую букву к верхнему регистру
            currentExercise = line.trimmed();
            currentExercise[0] = currentExercise[0].toUpper();
            if (!exercisesTonnage.contains(currentExercise)) {
                exercisesTonnage[currentExercise] = QList<double>();
            }
        }
    }



    return qMakePair(exercisesTonnage, date);
}
