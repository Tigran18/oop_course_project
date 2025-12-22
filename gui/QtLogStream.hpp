#pragma once

#include <QObject>
#include <QString>
#include <streambuf>

class QtLogStream : public QObject, public std::streambuf
{
    Q_OBJECT
public:
    explicit QtLogStream(QObject* parent = nullptr);

signals:
    void textReady(const QString& line);

protected:
    int overflow(int ch) override;
    int sync() override;

private:
    QString buffer_;
};
