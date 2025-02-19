#ifndef DBHANDLER_H
#define DBHANDLER_H

#include <QSqlDatabase>

class DbHandler
{
private:
    DbHandler();
    ~DbHandler();

    QSqlDatabase db;
    static DbHandler* instance;

private:
    void openDb();
    int getOrInsertExerciseId(const QString &exerciseName);

public:
    static DbHandler* getInstance();
    QSqlDatabase& getDb();

    void saveTrain(const QString& date, const QMap<QString, QList<double>> &trainInfo);
};

#endif // DBHANDLER_H
