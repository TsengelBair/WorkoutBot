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
    void createTables();
    int getOrInsertExerciseId(const QString &exerciseName);

public:
    static DbHandler* getInstance();
    QSqlDatabase& getDb();

    bool saveTrain(const QString& date, const QMap<QString, QList<double>> &trainInfo);
    QMap<QString, double> trainData();
};

#endif // DBHANDLER_H
