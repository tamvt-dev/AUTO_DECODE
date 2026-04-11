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

    struct CandidateResult {
        QString output;
        double score = 0.0;
        QString steps;
        // List of (Step Name, Step Output) for high-fidelity inspection
        QList<QPair<QString, QString>> history;
    };

    struct PipelineResult {
        QList<CandidateResult> candidates;
        bool success = false;
        QString errorMessage;
    };
    PipelineResult runPipeline(const QString &input, int maxDepth = 4, int beamWidth = 10);

    struct RecipeResult {
        QString output;
        bool success = false;
        QString error;
    };
    RecipeResult executeRecipe(const QString &jsonStr, const QString &input);

    void clearCache();

private:
    DecoderEngine() = default;
    void rebuildFormatLists();

    QStringList m_decodeFormats;
    QStringList m_encodeFormats;
};

#endif // DECODER_ENGINE_H