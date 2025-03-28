#ifndef PARSER_H
#define PARSER_H

#include <QString>
#include <QPair>

class Parser
{
public:
    Parser();

    static void init(QString& currentTrainData, QString firstExercise);
    static QString calcSetTonnage(const QString& input);
    static QPair<QMap<QString, QList<double>>, QString> parseWorkoutMessage(const QString &message);
    static QString makeCorrectStrCase(QString& str); /// для приведения первой буквы к верхнему, остальных к нижнему регистру
};

#endif // PARSER_H
