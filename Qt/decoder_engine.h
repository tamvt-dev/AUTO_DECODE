#ifndef DECODER_ENGINE_H
#define DECODER_ENGINE_H

#include <QString>
#include <QStringList>

class DecoderEngine
{
public:
    static DecoderEngine& instance();

    // No init/cleanup here – core is managed externally in main()
    void refreshFormats();

    QStringList decodeFormats() const { return m_decodeFormats; }
    QStringList encodeFormats() const { return m_encodeFormats; }

    struct DecodeResult {
        QString output;
        QString format;
        double processingTimeMs = 0.0;
        bool success = false;
        QString errorMessage;
    };
    DecodeResult decode(const QString &input, int formatIndex);

    struct EncodeResult {
        QString output;
        bool success = false;
        QString errorMessage;
    };
    EncodeResult encode(const QString &input, int formatIndex);

    struct PipelineResult {
        QString output;
        double score = 0.0;
        QString steps;
        bool success = false;
    };
    PipelineResult runPipeline(const QString &input, int maxDepth = 3, int beamWidth = 5);

    void clearCache();

private:
    DecoderEngine() = default;
    void rebuildFormatLists();

    QStringList m_decodeFormats;
    QStringList m_encodeFormats;
};

#endif // DECODER_ENGINE_H