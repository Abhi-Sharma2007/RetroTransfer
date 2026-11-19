#include "scanner.h"
#include "chunk_engine.h"
#include "chunk_config.h"
#include "compression_engine.h"
#include "reconstruction_engine.h"
#include <chrono>
#include <fstream>
#include <filesystem>
#include <iomanip>
#include <limits>
#include <cctype>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

static std::string jsonEscape(const std::string& value) {
    static constexpr char HEX[] = "0123456789abcdef";
    std::string escaped;
    escaped.reserve(value.size());
    for (char rawCharacter : value) {
        const auto character = static_cast<unsigned char>(rawCharacter);
        switch (character) {
            case '\\': escaped += "\\\\"; break;
            case '"':  escaped += "\\\""; break;
            case '\b': escaped += "\\b"; break;
            case '\f': escaped += "\\f"; break;
            case '\n': escaped += "\\n"; break;
            case '\r': escaped += "\\r"; break;
            case '\t': escaped += "\\t"; break;
            default:
                if (character < 0x20) {
                    escaped += "\\u00";
                    escaped += HEX[(character >> 4) & 0x0f];
                    escaped += HEX[character & 0x0f];
                } else {
                    escaped += static_cast<char>(character);
                }
        }
    }
    return escaped;
}

static bool parseSizeArgument(const std::string& text, size_t& value) {
    if (text.empty()) return false;
    for (char rawCharacter : text) {
        const auto character = static_cast<unsigned char>(rawCharacter);
        if (!std::isdigit(character)) return false;
    }
    try {
        const unsigned long long parsed = std::stoull(text);
        if (parsed == 0 || parsed > static_cast<unsigned long long>(
                std::numeric_limits<size_t>::max()) ||
            parsed > static_cast<unsigned long long>(
                std::numeric_limits<std::streamsize>::max())) {
            return false;
        }
        // A chunk is held in memory while it is fingerprinted. Keep CLI settings
        // bounded so a typo cannot request an effectively unbounded allocation.
        constexpr unsigned long long MAX_CHUNK_BYTES = 1024ULL * 1024ULL * 1024ULL;
        if (parsed > MAX_CHUNK_BYTES) return false;
        value = static_cast<size_t>(parsed);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

static void writeIndent(std::ofstream& output, int depth) {
    output << std::string(static_cast<size_t>(depth * 2), ' ');
}

static void writeDirectory(std::ofstream& output, const DirectoryNode& directory, int depth) {
    writeIndent(output, depth);
    output << "{\n";
    writeIndent(output, depth + 1);
    output << "\"name\": \"" << jsonEscape(directory.directoryName) << "\",\n";
    writeIndent(output, depth + 1);
    output << "\"path\": \"" << jsonEscape(directory.directoryPath) << "\",\n";
    writeIndent(output, depth + 1);
    output << "\"directories\": [\n";
    for (size_t index = 0; index < directory.directories.size(); ++index) {
        writeDirectory(output, directory.directories[index], depth + 2);
        if (index + 1 < directory.directories.size()) {
            output << ",";
        }
        output << "\n";
    }
    writeIndent(output, depth + 1);
    output << "],\n";
    writeIndent(output, depth + 1);
    output << "\"files\": [\n";
    for (size_t index = 0; index < directory.files.size(); ++index) {
        const FileInfo& file = directory.files[index];
        writeIndent(output, depth + 2);
        output << "{\"name\": \"" << jsonEscape(file.fileName)
               << "\", \"path\": \"" << jsonEscape(file.filePath)
               << "\", \"size\": " << file.fileSize
               << ", \"type\": \"" << jsonEscape(file.fileType)
               << "\", \"modificationTime\": " << file.modificationTime << "}";
        if (index + 1 < directory.files.size()) {
            output << ",";
        }
        output << "\n";
    }
    writeIndent(output, depth + 1);
    output << "]\n";
    writeIndent(output, depth);
    output << "}";
}

static void writeChunkManifestJson(
    std::ofstream& out,
    const ProjectChunkResult& result,
    const EngineConfig& config,
    const std::string& rootPath,
    const CompressionResult& compression,
    const std::string& payloadFilename)
{
    out << "{\n";

    out << "  \"rootPath\": \"" << jsonEscape(rootPath) << "\",\n";
    out << "  \"selectedStrategy\": \""
        << strategy_to_string(config.strategy) << "\",\n";

    // Chunking configuration
    out << "  \"chunking\": {\n";
    out << "    \"fixedChunkSize\": "
        << config.fixedConfig.chunkSize << ",\n";
    out << "    \"cdcMinChunkSize\": "
        << config.cdcConfig.minChunkSize << ",\n";
    out << "    \"cdcTargetChunkSize\": "
        << config.cdcConfig.targetChunkSize << ",\n";
    out << "    \"cdcMaxChunkSize\": "
        << config.cdcConfig.maxChunkSize << "\n";
    out << "  },\n";

    // Compression information
    out << "  \"selectiveCompression\": {\n";
    out << "    \"payloadFile\": \""
        << jsonEscape(payloadFilename) << "\",\n";
    out << "    \"payloadChunks\": [\n";

    for (size_t i = 0; i < compression.payloadChunks.size(); ++i) {
        const auto& chunk = compression.payloadChunks[i];

        out << "      {\n";
        out << "        \"fingerprint\": \""
            << jsonEscape(chunk.metadata.fingerprint) << "\",\n";
        out << "        \"originalSize\": "
            << chunk.metadata.length << ",\n";
        out << "        \"payloadOffset\": "
            << chunk.payloadOffset << ",\n";
        out << "        \"storedSize\": "
            << chunk.storedSize << ",\n";
        out << "        \"crc32\": "
            << chunk.metadata.crc32 << ",\n";
        out << "        \"codec\": \""
            << jsonEscape(chunk.codec) << "\"\n";
        out << "      }";

        if (i + 1 < compression.payloadChunks.size()) {
            out << ",";
        }
        out << "\n";
    }

    out << "    ]\n";
    out << "  },\n";

    // Existing chunking summary
    out << "  \"summary\": {\n";
    out << "    \"totalFiles\": "
        << result.metrics.totalFilesProcessed << ",\n";
    out << "    \"totalBytes\": "
        << result.metrics.totalBytesProcessed << ",\n";
    out << "    \"totalChunks\": "
        << result.metrics.totalChunksGenerated << ",\n";
    out << "    \"uniqueFingerprints\": "
        << result.metrics.uniqueFingerprints << ",\n";
    out << "    \"deduplicatedBytes\": "
        << result.metrics.deduplicatedBytes << ",\n";
    out << "    \"internalDedupRatio\": "
        << std::fixed << std::setprecision(4)
        << result.metrics.internalDedupRatio << ",\n";
    out << "    \"totalChunkingTimeMs\": "
        << std::fixed << std::setprecision(2)
        << result.metrics.totalChunkingTimeMs << ",\n";
    out << "    \"totalFingerprintingTimeMs\": "
        << result.metrics.totalFingerprintingTimeMs << ",\n";
    out << "    \"totalExecutionTimeMs\": "
        << result.metrics.totalExecutionTimeMs << ",\n";
    out << "    \"overallThroughputMBps\": "
        << result.metrics.overallThroughputMBps << ",\n";
    out << "    \"estimatedMetadataBytes\": "
        << result.metrics.totalEstimatedMetadataBytes << "\n";
    out << "  },\n";

    // File reconstruction recipes
    out << "  \"files\": [\n";

    for (size_t i = 0; i < result.files.size(); ++i) {
        const auto& f = result.files[i];

        out << "    {\n";
        out << "      \"path\": \""
            << jsonEscape(f.relativePath) << "\",\n";
        out << "      \"size\": " << f.metrics.fileSize << ",\n";
        out << "      \"strategy\": \""
            << f.metrics.selectedStrategy << "\",\n";
        out << "      \"reason\": \""
            << jsonEscape(f.decision.reason) << "\",\n";
        out << "      \"chunkCount\": "
            << f.chunks.size() << ",\n";
        out << "      \"avgChunkSize\": "
            << std::fixed << std::setprecision(1)
            << f.metrics.avgChunkSize << ",\n";
        out << "      \"minChunkSize\": "
            << f.metrics.minChunkSize << ",\n";
        out << "      \"maxChunkSize\": "
            << f.metrics.maxChunkSize << ",\n";
        out << "      \"totalTimeMs\": "
            << std::fixed << std::setprecision(3)
            << f.metrics.totalDurationMs << ",\n";

        out << "      \"chunks\": [\n";

        for (size_t c = 0; c < f.chunks.size(); ++c) {
            const auto& chunk = f.chunks[c];

            out << "        {"
                << "\"seq\": " << chunk.sequenceNumber
                << ", \"offset\": " << chunk.offset
                << ", \"length\": " << chunk.length
                << ", \"fingerprint\": \""
                << jsonEscape(chunk.fingerprint) << "\""
                << ", \"crc32\": " << chunk.crc32
                << ", \"algorithm\": \""
                << jsonEscape(chunk.algorithm) << "\"}";

            if (c + 1 < f.chunks.size()) {
                out << ",";
            }
            out << "\n";
        }

        out << "      ]\n";
        out << "    }";

        if (i + 1 < result.files.size()) {
            out << ",";
        }
        out << "\n";
    }

    out << "  ]\n";
    out << "}\n";
}
static void printUsage(const char* progName) {
    std::cout << "RetroTransfer - Project Scanner and Adaptive Chunking Engine\n";
    std::cout << "Usage:\n";
    std::cout << "  " << progName << " [directory] [output.json] [options]\n\n";
    std::cout << "Arguments:\n";
    std::cout << "  [directory]           Root directory to scan (default: \".\")\n";
    std::cout << "  [output.json]         Filename for scan output inside collection/ (default: \"scan.json\")\n\n";
    std::cout << "Options:\n";
    std::cout << "  --strategy, -s <mode> Chunking strategy: \"adaptive\" (default), \"fixed\", or \"cdc\"\n";
    std::cout << "  --chunk-size <bytes>  Fixed chunk size in bytes (default: 65536)\n";
    std::cout << "  --min-chunk <bytes>   CDC minimum chunk size (default: 16384)\n";
    std::cout << "  --target-chunk <bytes> CDC target chunk size (default: 65536)\n";
    std::cout << "  --max-chunk <bytes>   CDC maximum chunk size (default: 262144)\n";
    std::cout << "  --benchmark           Run repeatable benchmark comparing Fixed, CDC, and Adaptive\n";
    std::cout << "  --no-chunk            Only perform project scan, skip chunking stage\n";
    std::cout << "  --help, -h            Show this help message\n";
}

static void runBenchmark(const DirectoryNode& directoryTree,
                         const std::string& path,
                         const EngineConfig& baseConfig) {
    std::cout << "\n============================================================\n";
    std::cout << "        RETROTRANSFER CHUNKING STRATEGY BENCHMARK           \n";
    std::cout << "============================================================\n";

    std::vector<ChunkStrategy> strategies = {
        ChunkStrategy::Fixed,
        ChunkStrategy::CDC,
        ChunkStrategy::Adaptive
    };

    struct BenchRow {
        std::string name;
        size_t chunks;
        size_t uniqueFp;
        uint64_t dedupBytes;
        double dedupRatio;
        double chunkTimeMs;
        double fpTimeMs;
        double totalTimeMs;
        double throughputMBps;
        uint64_t metadataBytes;
    };

    std::vector<BenchRow> rows;

    for (auto strat : strategies) {
        EngineConfig cfg = baseConfig;
        cfg.strategy = strat;
        ChunkEngine engine(cfg);

        ProjectChunkResult res = engine.processScannedProject(directoryTree, path);
        BenchRow row;
        row.name = strategy_to_string(strat);
        row.chunks = res.metrics.totalChunksGenerated;
        row.uniqueFp = res.metrics.uniqueFingerprints;
        row.dedupBytes = res.metrics.deduplicatedBytes;
        row.dedupRatio = res.metrics.internalDedupRatio * 100.0;
        row.chunkTimeMs = res.metrics.totalChunkingTimeMs;
        row.fpTimeMs = res.metrics.totalFingerprintingTimeMs;
        row.totalTimeMs = res.metrics.totalExecutionTimeMs;
        row.throughputMBps = res.metrics.overallThroughputMBps;
        row.metadataBytes = res.metrics.totalEstimatedMetadataBytes;
        rows.push_back(row);
    }

    std::cout << std::left 
              << std::setw(12) << "Strategy"
              << std::setw(10) << "Chunks"
              << std::setw(12) << "Unique FPs"
              << std::setw(12) << "Dedup %"
              << std::setw(14) << "Time (ms)"
              << std::setw(18) << "Throughput(MB/s)"
              << std::setw(16) << "Metadata (KB)"
              << "\n";
    std::cout << std::string(84, '-') << "\n";

    for (const auto& r : rows) {
        std::cout << std::left 
                  << std::setw(12) << r.name
                  << std::setw(10) << r.chunks
                  << std::setw(12) << r.uniqueFp
                  << std::setw(12) << (std::to_string(static_cast<int>(r.dedupRatio * 10) / 10.0) + "%")
                  << std::setw(14) << std::fixed << std::setprecision(2) << r.totalTimeMs
                  << std::setw(18) << std::fixed << std::setprecision(2) << r.throughputMBps
                  << std::setw(16) << (r.metadataBytes / 1024)
                  << "\n";
    }
    std::cout << "============================================================\n\n";
}

int runBackend(int argc, char* argv[]) {
    std::string path = ".";
    std::string outputName = "scan.json";
    bool enableChunking = true;
    bool benchmarkMode = false;

    EngineConfig config;

    // Parse command-line arguments and fail clearly on malformed values.
    int positionalIdx = 0;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "--no-chunk") {
            enableChunking = false;
        } else if (arg == "--benchmark") {
            benchmarkMode = true;
        } else if (arg == "--strategy" || arg == "-s") {
            if (i + 1 >= argc) {
                std::cerr << "Error: " << arg << " requires a strategy value.\n";
                return 2;
            }
            const std::string strategyText = argv[++i];
            if (!string_to_strategy(strategyText, config.strategy)) {
                std::cerr << "Error: Unknown strategy '" << strategyText
                          << "'. Choose adaptive, fixed, or cdc.\n";
                return 2;
            }
        } else if (arg == "--chunk-size" || arg == "--min-chunk" ||
                   arg == "--target-chunk" || arg == "--max-chunk") {
            if (i + 1 >= argc) {
                std::cerr << "Error: " << arg << " requires a positive byte count (max 1 GiB).\n";
                return 2;
            }
            size_t parsed = 0;
            const std::string valueText = argv[++i];
            if (!parseSizeArgument(valueText, parsed)) {
                std::cerr << "Error: Invalid value '" << valueText << "' for " << arg
                          << ". Use a positive integer up to 1073741824 bytes.\n";
                return 2;
            }
            if (arg == "--chunk-size") {
                config.fixedConfig.chunkSize = parsed;
            } else if (arg == "--min-chunk") {
                config.cdcConfig.minChunkSize = parsed;
                config.cdcConfig.update_mask();
            } else if (arg == "--target-chunk") {
                config.cdcConfig.targetChunkSize = parsed;
                config.cdcConfig.update_mask();
            } else {
                config.cdcConfig.maxChunkSize = parsed;
            }
        } else if (arg.rfind("--", 0) == 0) {
            std::cerr << "Error: Unrecognized option: " << arg << "\n";
            return 2;
        } else {
            if (positionalIdx == 0) {
                path = arg;
                ++positionalIdx;
            } else if (positionalIdx == 1) {
                outputName = arg;
                ++positionalIdx;
            } else {
                std::cerr << "Error: Unexpected positional argument: " << arg << "\n";
                return 2;
            }
        }
    }

    if (!config.fixedConfig.isValid() || !config.cdcConfig.isValid()) {
        std::cerr << "Error: Invalid chunk configuration. Require 0 < min <= target <= max; "
                     "all sizes must be at most 1 GiB.\n";
        return 2;
    }

    std::error_code rootError;
    if (!fs::exists(path, rootError) || rootError || !fs::is_directory(path, rootError) || rootError) {
        std::cerr << "Error: Scan root is not an accessible directory: " << path << "\n";
        return 1;
    }

    const fs::path collectionFolder = "collection";
    const fs::path outputPath = collectionFolder / fs::path(outputName).filename();

    // 1. Project Scanner Stage
    std::cout << "[Stage 1/2] Scanning directory: " << path << " ...\n";
    ProjectScanner scanner(path);
    const DirectoryNode directoryTree = scanner.scan();

    std::error_code outputDirError;
    fs::create_directories(collectionFolder, outputDirError);
    if (outputDirError) {
        std::cerr << "Error: Could not create output directory " << collectionFolder
                  << ": " << outputDirError.message() << "\n";
        return 1;
    }
    std::ofstream output(outputPath);
    if (!output) {
        std::cerr << "Error: Could not create output file: " << outputPath << "\n";
        return 1;
    }

    writeDirectory(output, directoryTree, 0);
    output << "\n";
    output.close();
    if (!output) {
        std::cerr << "Error: Failed while writing scan output: " << outputPath << "\n";
        return 1;
    }
    std::cout << "Saved scan data to " << outputPath << "\n";

    // 2. Chunking Engine Stage
    if (enableChunking) {
        std::cout << "[Stage 2/2] Running Chunking Engine (Mode: " 
                  << strategy_to_string(config.strategy) << ") ...\n";

        ChunkEngine engine(config);
        ProjectChunkResult chunkResult = engine.processScannedProject(directoryTree, path);
// ------------------------------------------
// COMPRESSION ENGINE
// ------------------------------------------

std::cout << "\n--- Starting Compression Engine ---\n";

ProjectChunkResult baseline;
CompressionResult compressionResult;
CompressionEngine compressionEngine;

std::string compressionError;

fs::path payloadFilename =
    outputPath.stem().string() + "_chunks.rtpk";

fs::path payloadPath = collectionFolder / payloadFilename;

bool compressionSuccess = compressionEngine.compressMissingChunks(
    chunkResult,
    baseline,
    payloadPath.string(),
    compressionResult,
    compressionError
);

if (!compressionSuccess) {
    std::cerr << "Compression failed: "
              << compressionError << "\n";
    return 1;
}

std::cout << "Compression completed successfully!\n";
std::cout << "Total chunks       : "
          << compressionResult.metrics.totalChunks << "\n";
std::cout << "Huffman chunks     : "
          << compressionResult.metrics.huffmanChunks << "\n";
std::cout << "Raw chunks         : "
          << compressionResult.metrics.storedRawChunks << "\n";
std::cout << "Payload size       : "
          << compressionResult.metrics.payloadBytes << " bytes\n";
std::cout << "-----------------------------------\n";
       
        // Save chunk manifest
        fs::path chunkOutFilename =
            outputPath.stem().string() + "_chunks.json";

        fs::path chunkOutputPath =
            collectionFolder / chunkOutFilename;

        std::ofstream chunkOut(chunkOutputPath);

        if (!chunkOut) {
            std::cerr << "Error: Could not create chunk manifest: "
                      << chunkOutputPath << "\n";
            return 1;
        }

        writeChunkManifestJson(
            chunkOut,
            chunkResult,
            config,
            path,
            compressionResult,
            payloadFilename.string()
        );

        chunkOut.close();

        if (!chunkOut) {
            std::cerr << "Error: Failed while writing chunk manifest: "
                      << chunkOutputPath << "\n";
            return 1;
        }

        std::cout << "Saved chunk manifest to "
                  << chunkOutputPath << "\n";


        std::cout << "\n--- Chunking Engine Summary ---\n";
        std::cout << "Files processed    : " << chunkResult.metrics.totalFilesProcessed << "\n";
        std::cout << "Total bytes        : " << chunkResult.metrics.totalBytesProcessed << " bytes ("
                  << std::fixed << std::setprecision(2)
                  << (static_cast<double>(chunkResult.metrics.totalBytesProcessed) / (1024.0 * 1024.0)) << " MB)\n";
        std::cout << "Chunks generated   : " << chunkResult.metrics.totalChunksGenerated << "\n";
        std::cout << "Unique fingerprints: " << chunkResult.metrics.uniqueFingerprints << "\n";
        std::cout << "Internal dedup bytes: " << chunkResult.metrics.deduplicatedBytes << " bytes ("
                  << std::fixed << std::setprecision(2)
                  << (chunkResult.metrics.internalDedupRatio * 100.0) << "%)\n";
        std::cout << "Chunking time      : " << std::fixed << std::setprecision(2)
                  << chunkResult.metrics.totalChunkingTimeMs << " ms\n";
        std::cout << "Fingerprinting time: " << std::fixed << std::setprecision(2)
                  << chunkResult.metrics.totalFingerprintingTimeMs << " ms\n";
        std::cout << "Total engine time  : " << std::fixed << std::setprecision(2)
                  << chunkResult.metrics.totalExecutionTimeMs << " ms\n";
        std::cout << "Overall throughput : " << std::fixed << std::setprecision(2)
                  << chunkResult.metrics.overallThroughputMBps << " MB/s\n";
        std::cout << "--------------------------------\n";
                // ------------------------------------------
        // RECONSTRUCTION ENGINE
        // ------------------------------------------

        std::cout << "\n--- Starting Reconstruction Engine ---\n";

        ReconstructionEngine reconstructionEngine;
        ReconstructionResult reconstructionResult;
        std::string reconstructionError;

        fs::path reconstructedFolder = "reconstructed_output";

        bool reconstructionSuccess = reconstructionEngine.reconstruct(
            chunkOutputPath.string(),
            payloadPath.string(),
            reconstructedFolder.string(),
            reconstructionResult,
            reconstructionError
        );

        if (!reconstructionSuccess) {
            std::cerr << "Reconstruction failed: "
                      << reconstructionError << "\n";
            return 1;
        }

        std::cout << "Reconstruction completed successfully!\n";
        std::cout << "Files written  : "
                  << reconstructionResult.filesWritten << "\n";
        std::cout << "Chunks written : "
                  << reconstructionResult.chunksWritten << "\n";
        std::cout << "Chunks reused  : "
                  << reconstructionResult.chunksReused << "\n";
        std::cout << "Output folder  : "
                  << reconstructedFolder << "\n";
        std::cout << "-----------------------------------\n";
        if (benchmarkMode) {
            runBenchmark(directoryTree, path, config);
        }
    }

    return 0;
} 

int executeBackend(const std::vector<std::string>& arguments)
{
    std::vector<char*> argv;

    for (const auto& argument : arguments) {
        argv.push_back(const_cast<char*>(argument.c_str()));
    }

    return runBackend(
        static_cast<int>(argv.size()),
        argv.data()
    );
}

void showMainMenu()
{
    std::cout << "\n";
    std::cout << "============================================\n";
    std::cout << "              RETRO TRANSFER                \n";
    std::cout << "          LOCAL FILE TRANSFER SYSTEM        \n";
    std::cout << "============================================\n";
    std::cout << "                 MAIN MENU                  \n";
    std::cout << "--------------------------------------------\n";
    std::cout << "  1. Start New Transfer\n";
    std::cout << "  2. Configure Chunking Strategy\n";
    std::cout << "  3. View Collection Files\n";
    std::cout << "  4. Project Information\n";
    std::cout << "  0. Exit\n";
    std::cout << "--------------------------------------------\n";
    std::cout << "Enter your choice: ";
}

void viewCollection()
{
    fs::path collectionPath = "collection";

    if (!fs::exists(collectionPath)) {
        std::cout << "\nNo collection folder found yet.\n";
        return;
    }

    std::cout << "\n========== COLLECTION FILES ==========\n";

    for (const auto& entry : fs::directory_iterator(collectionPath)) {
        if (entry.is_regular_file()) {
            std::cout << entry.path().filename()
                      << "  (" << entry.file_size() << " bytes)\n";
        }
    }

    std::cout << "======================================\n";
}


int main(int argc, char* argv[])
{
    // If arguments are supplied, run backend directly for the GUI.
    if (argc > 1) {
        return runBackend(argc, argv);
    }

    // Otherwise, launch the normal terminal menu.
    std::string selectedStrategy = "adaptive";

    while (true)
    {
        showMainMenu();

        int choice;

        if (!(std::cin >> choice)) {
            std::cin.clear();
            std::cin.ignore(
                std::numeric_limits<std::streamsize>::max(),
                '\n'
            );

            std::cout << "\nInvalid input. Enter a number.\n";
            continue;
        }

        std::cin.ignore(
            std::numeric_limits<std::streamsize>::max(),
            '\n'
        );

        switch (choice)
        {
            case 1:
            {
                std::string folderPath;

                std::cout << "\nEnter folder path: ";
                std::getline(std::cin, folderPath);

                // Remove surrounding quotes if pasted from Windows Explorer.
                if (folderPath.size() >= 2 &&
                    folderPath.front() == '"' &&
                    folderPath.back() == '"') {
                    folderPath = folderPath.substr(
                        1, folderPath.size() - 2
                    );
                }

                if (folderPath.empty() ||
                    !fs::exists(folderPath) ||
                    !fs::is_directory(folderPath)) {
                    std::cout << "\nInvalid folder path!\n";
                    break;
                }

                std::cout << "\nStarting RetroTransfer...\n";

                executeBackend({
                    "RetroTransfer",
                    folderPath,
                    "--strategy",
                    selectedStrategy
                });

                std::cout << "\nReturning to main menu...\n";
                break;
            }

            case 2:
            {
                std::cout << "\n===== CHUNKING STRATEGY =====\n";
                std::cout << "1. Adaptive\n";
                std::cout << "2. Fixed\n";
                std::cout << "3. CDC\n";
                std::cout << "Enter choice: ";

                int strategyChoice;

                if (!(std::cin >> strategyChoice)) {
                    std::cin.clear();
                    std::cin.ignore(
                        std::numeric_limits<std::streamsize>::max(),
                        '\n'
                    );
                    std::cout << "Invalid input.\n";
                    break;
                }

                std::cin.ignore(
                    std::numeric_limits<std::streamsize>::max(),
                    '\n'
                );

                switch (strategyChoice)
                {
                    case 1:
                        selectedStrategy = "adaptive";
                        break;

                    case 2:
                        selectedStrategy = "fixed";
                        break;

                    case 3:
                        selectedStrategy = "cdc";
                        break;

                    default:
                        std::cout << "Invalid strategy choice.\n";
                        continue;
                }

                std::cout << "Strategy selected: "
                          << selectedStrategy << "\n";
                break;
            }

            case 3:
                viewCollection();
                break;

            case 4:
                std::cout << "\n========== PROJECT INFORMATION ==========\n";
                std::cout << "Project: RetroTransfer\n";
                std::cout << "Version: 1.0\n";
                std::cout << "Language: C++17 and C\n";
                std::cout << "Modules:\n";
                std::cout << "- Project Scanner\n";
                std::cout << "- Chunking Engine\n";
                std::cout << "- Fingerprinting\n";
                std::cout << "- Compression Engine\n";
                std::cout << "- Reconstruction Engine\n";
                std::cout << "Networking: Planned for a future version\n";
                std::cout << "=========================================\n";
                break;

            case 0:
                std::cout << "\nThank you for using RetroTransfer!\n";
                return 0;

            default:
                std::cout << "\nInvalid choice. Try again.\n";
        }
    }
}
