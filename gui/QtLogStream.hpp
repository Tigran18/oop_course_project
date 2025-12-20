#pragma once

#include <QObject>
#include <QString>
#include <QChar>
#include <streambuf>

/*
 * Redirects std::cout/std::cerr into Qt via signal.
 * Emits complete lines (on '\n'), and also supports bulk writes (xsputn).
 */
class QtLogStream : public QObject, public std::basic_streambuf<char> {
    Q_OBJECT
public:
    explicit QtLogStream(QObject* parent = nullptr);

signals:
    void textReady(const QString& line);

protected:
    int overflow(int ch) override;
    std::streamsize xsputn(const char* s, std::streamsize n) override;

private:
    void flushLineIfNeeded();
    QString buffer_;
};
