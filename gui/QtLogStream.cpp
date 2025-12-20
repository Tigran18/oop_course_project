#include "gui/QtLogStream.hpp"

QtLogStream::QtLogStream(QObject* parent)
    : QObject(parent)
{
}

int QtLogStream::overflow(int ch)
{
    if (ch == EOF) return EOF;

    const QChar qc = QChar(static_cast<char>(ch));
    if (qc == QChar('\n')) {
        flushLineIfNeeded();
    } else {
        buffer_ += qc;
    }
    return ch;
}

std::streamsize QtLogStream::xsputn(const char* s, std::streamsize n)
{
    for (std::streamsize i = 0; i < n; ++i) {
        overflow(static_cast<unsigned char>(s[i]));
    }
    return n;
}

void QtLogStream::flushLineIfNeeded()
{
    if (!buffer_.isEmpty()) {
        emit textReady(buffer_);
        buffer_.clear();
    } else {
        // empty line
        emit textReady(QString());
    }
}
