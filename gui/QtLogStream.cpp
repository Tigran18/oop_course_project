#include "gui/QtLogStream.hpp"

QtLogStream::QtLogStream(QObject* parent)
    : QObject(parent)
{
}

int QtLogStream::overflow(int ch)
{
    if (ch == traits_type::eof())
        return traits_type::not_eof(ch);

    const QChar qc(static_cast<char>(ch));

    if (qc == '\n') {
        if (!buffer_.isEmpty())
            emit textReady(buffer_);
        buffer_.clear();
    } else if (qc != '\r') {
        buffer_.append(qc);
    }

    return ch;
}

int QtLogStream::sync()
{
    if (!buffer_.isEmpty()) {
        emit textReady(buffer_);
        buffer_.clear();
    }
    return 0;
}
