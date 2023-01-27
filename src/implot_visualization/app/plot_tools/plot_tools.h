#pragma once

#include <deserialize_json.h>
#include <IconsFontAwesome6.h>
#include <implot.h>
#include <vector>

namespace Plotter {

enum DataInterval { Short,
    Mid,
    Long };

/**
 * Adds an overlay to the current plot and shows an icon and a message.
 * @param text the message text shown after the icon
 * @param fontawesome the icon font to use
 * @param icon the icon to print
 * @param icon_color the color to draw the icon with
 * @param relPos optional relative position inside the plot bounds
 */
void addPlotNotice(
        const std::string_view text,
        ImFont                *fontawesome,
        const std::string_view icon       = ICON_FA_TRIANGLE_EXCLAMATION,
        const ImVec4           icon_color = { 1.0, 0, 0, 1.0 }, // red
        const ImVec2           relPos     = { 0.15, 0.05 }) {
    const auto plot_size = ImPlot::GetPlotSize();
    const auto plot_pos  = ImPlot::GetPlotPos();
    const auto pos       = ImVec2(plot_pos.x + plot_size.x * relPos.x, plot_pos.y + plot_size.y * relPos.y);
    ImGui::SetNextWindowPos(pos);
    ImGui::PushFont(fontawesome);
    const auto iconSize = ImGui::CalcTextSize(icon.data(), icon.data() + icon.size());
    ImGui::PopFont();
    const auto   textSize = ImGui::CalcTextSize(text.data(), text.data() + text.size());
    ImVec2      &spacing  = ImGui::GetStyle().ItemSpacing;
    const ImVec2 overlaySize{ iconSize.x + textSize.x + 2 * spacing.x, std::max(iconSize.y, textSize.y) + 2 * spacing.y };
    if (ImGui::BeginChild(text.data(), overlaySize, false,
                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoBackground)) {
        ImGui::PushFont(fontawesome);
        ImGui::TextColored(icon_color, icon.data(), icon.data() + icon.size());
        ImGui::PopFont();
        ImGui::SameLine();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + iconSize.y - textSize.y); // bottom align
        ImGui::Text("%.*s", static_cast<int>(text.size()), text.data());
    }
    ImGui::EndChild();
}

double setTimeInterval(DataInterval Interval) {
    double dt = 0;
    switch (Interval) {
    case Short:
        dt = 300.0; // 5 minutes
        break;
    case Mid:
        dt = 3'600.0; // 1 hour
        break;
    case Long:
        dt = 86'400.0; // 24 hours
        break;
    }
    return dt;
}

template<typename T>
static void plotSignals(std::vector<T> &signals) {
    for (const auto &signal : signals) {
        if (!signal.data.empty()) {
            int offset = 0;
            if constexpr (requires { signal.offset; }) {
                offset = signal.offset;
            }
            ImPlot::PlotLine((signal.signalName).c_str(),
                    &signal.data[0].x,
                    &signal.data[0].y,
                    signal.data.size(),
                    0,
                    offset,
                    2 * sizeof(double));
        }
    }
}

void plotGrSignals(std::vector<ScrollingBuffer> &signals) {
    static ImPlotAxisFlags xflags = ImPlotAxisFlags_None;
    static ImPlotAxisFlags yflags = ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_RangeFit;
    ImPlot::SetupAxes("UTC Time", "U(V)", xflags, yflags);
    auto   clock       = std::chrono::system_clock::now();
    double currentTime = static_cast<double>(std::chrono::duration_cast<std::chrono::seconds>(clock.time_since_epoch()).count());
    ImPlot::SetupAxisLimits(ImAxis_X1, currentTime - 0.06, currentTime, ImGuiCond_Always);
    ImPlot::SetupAxisScale(ImAxis_X1, ImPlotScale_Time);
    ImPlot::SetupAxis(ImAxis_Y2, "I(A)", ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_RangeFit | ImPlotAxisFlags_AuxDefault);
    ImPlot::SetNextFillStyle(IMPLOT_AUTO_COL, 0.5f);
    for (const auto &signal : signals) {
        if (!signal.data.empty()) {
            int offset = 0;
            if constexpr (requires { signal.offset; }) {
                offset = signal.offset;
            }
            if (signal.signalName.find("I") != std::string::npos) {
                ImPlot::SetAxes(ImAxis_X1, ImAxis_Y2);
            }
            ImPlot::PlotLine((signal.signalName).c_str(),
                    &signal.data[0].x,
                    &signal.data[0].y,
                    signal.data.size(),
                    0,
                    offset,
                    2 * sizeof(double));

            ImPlot::SetAxes(ImAxis_X1, ImAxis_Y1);
        }
    }
}

void plotBandpassFilter(std::vector<ScrollingBuffer> &signals) {
    static ImPlotAxisFlags xflags = ImPlotAxisFlags_None;
    static ImPlotAxisFlags yflags = ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_RangeFit;
    ImPlot::SetupAxes("UTC Time", "U(V)", xflags, yflags);
    auto   clock       = std::chrono::system_clock::now();
    double currentTime = static_cast<double>(std::chrono::duration_cast<std::chrono::seconds>(clock.time_since_epoch()).count());
    ImPlot::SetupAxisLimits(ImAxis_X1, currentTime - 0.06, currentTime, ImGuiCond_Always);
    ImPlot::SetupAxisScale(ImAxis_X1, ImPlotScale_Time);
    ImPlot::SetupAxis(ImAxis_Y2, "I(A)", ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_RangeFit | ImPlotAxisFlags_AuxDefault);
    ImPlot::SetNextFillStyle(IMPLOT_AUTO_COL, 0.5f);
    for (const auto &signal : signals) {
        if (!signal.data.empty()) {
            int offset = 0;
            if constexpr (requires { signal.offset; }) {
                offset = signal.offset;
            }
            if (signal.signalName.find("I") != std::string::npos) {
                ImPlot::SetAxes(ImAxis_X1, ImAxis_Y2);
            }
            ImPlot::PlotLine((signal.signalName).c_str(),
                    &signal.data[0].x,
                    &signal.data[0].y,
                    signal.data.size(),
                    0,
                    offset,
                    2 * sizeof(double));
        }
    }
}

void plotPower(std::vector<ScrollingBuffer> &signals, DataInterval Interval) {
    static ImPlotAxisFlags xflags = ImPlotAxisFlags_None;
    static ImPlotAxisFlags yflags = ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_RangeFit;
    ImPlot::SetupAxes("UTC Time", "P(W), Q(Var), S(VA)", xflags, yflags);
    auto   clock       = std::chrono::system_clock::now();
    double currentTime = static_cast<double>(std::chrono::duration_cast<std::chrono::seconds>(clock.time_since_epoch()).count());
    double dt          = setTimeInterval(Interval);
    ImPlot::SetupAxisLimits(ImAxis_X1, currentTime - dt, currentTime, ImGuiCond_Always);
    ImPlot::SetupAxisScale(ImAxis_X1, ImPlotScale_Time);
    ImPlot::SetupAxis(ImAxis_Y2, "phi(rad)", ImPlotAxisFlags_AuxDefault);
    ImPlot::SetNextFillStyle(IMPLOT_AUTO_COL, 0.5f);
    for (const auto &signal : signals) {
        if (!signal.data.empty()) {
            int offset = 0;
            if constexpr (requires { signal.offset; }) {
                offset = signal.offset;
            }
            if (signal.signalName.find("phi") != std::string::npos) {
                ImPlot::SetAxes(ImAxis_X1, ImAxis_Y2);
            }
            ImPlot::PlotLine((signal.signalName).c_str(),
                    &signal.data[0].x,
                    &signal.data[0].y,
                    signal.data.size(),
                    0,
                    offset,
                    2 * sizeof(double));

            // Add tags with signal value
            DataPoint lastPoint = signal.data.back();
            ImVec4    col       = ImPlot::GetLastItemColor();
            ImPlot::TagY(lastPoint.y, col, "%2f", lastPoint.y);
        }
    }
}

void plotStatistics(std::vector<ScrollingBuffer> &signals, DataInterval Interval) {
    static ImPlotAxisFlags xflags = ImPlotAxisFlags_None;
    static ImPlotAxisFlags yflags = ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_RangeFit;
    ImPlot::SetupAxes("UTC Time", "P(W), Q(Var), S(VA)", xflags, yflags);
    auto   clock       = std::chrono::system_clock::now();
    double currentTime = static_cast<double>(std::chrono::duration_cast<std::chrono::seconds>(clock.time_since_epoch()).count());
    double dt          = setTimeInterval(Interval);
    ImPlot::SetupAxisLimits(ImAxis_X1, currentTime - dt, currentTime, ImGuiCond_Always);
    ImPlot::SetupAxis(ImAxis_Y2, "phi(rad)", ImPlotAxisFlags_AuxDefault);
    ImPlot::SetupAxisScale(ImAxis_X1, ImPlotScale_Time);
    ImPlot::SetNextFillStyle(IMPLOT_AUTO_COL, 0.5f);

    for (auto signal : signals) {
        if (signal.data.empty()) {
            return;
        }
    }

    static float alpha = 0.25f;
    ImGui::DragFloat("Alpha", &alpha, 0.01f, 0, 1);
    if (signals.size() == 12) {
        int offset = 0;
        if constexpr (requires { signals[0].offset; }) {
            offset = signals[0].offset;
        }

        // Plot min and max as shaded
        ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, alpha);
        for (int i = 0; i < 12; i += 3) {
            if (signals[i].signalName.find("phi") != std::string::npos) {
                ImPlot::SetAxes(ImAxis_X1, ImAxis_Y2);
            }
            ImPlot::PlotShaded((signals[i].signalName).c_str(),
                    &signals[i].data[0].x,
                    &signals[i + 2].data[0].y,
                    &signals[i + 1].data[0].y,
                    signals[i].data.size(),
                    0,
                    offset,
                    2 * sizeof(double));
            ImPlot::SetAxes(ImAxis_X1, ImAxis_Y1);
        }
        ImPlot::PopStyleVar();

        // Plot mean as line
        for (int i = 0; i < 12; i += 3) {
            if (signals[i].signalName.find("phi") != std::string::npos) {
                ImPlot::SetAxes(ImAxis_X1, ImAxis_Y2);
            }
            // ImPlot::PushStyleColor(ImPlotCol_Line, col);
            ImPlot::PlotLine((signals[i].signalName).c_str(),
                    &signals[i].data[0].x,
                    &signals[i].data[0].y,
                    signals[i].data.size(),
                    0,
                    offset,
                    2 * sizeof(double));

            // Add tags with signal value
            ImVec4    col       = ImPlot::GetLastItemColor();
            DataPoint lastPoint = signals[i].data.back();
            ImPlot::TagY(lastPoint.y, col, "%2f", lastPoint.y);
            ImPlot::SetAxes(ImAxis_X1, ImAxis_Y1);
        }
    }
}

void plotMainsFrequency(std::vector<ScrollingBuffer> &signals, DataInterval Interval) {
    static ImPlotAxisFlags xflags = ImPlotAxisFlags_None;
    static ImPlotAxisFlags yflags = ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_RangeFit;
    ImPlot::SetupAxes("UTC Time", "Frequency (Hz)", xflags, yflags);
    auto   clock       = std::chrono::system_clock::now();
    double currentTime = static_cast<double>(std::chrono::duration_cast<std::chrono::seconds>(clock.time_since_epoch()).count());
    double dt          = setTimeInterval(Interval);
    ImPlot::SetupAxisLimits(ImAxis_X1, currentTime - dt, currentTime, ImGuiCond_Always);
    ImPlot::SetupAxisScale(ImAxis_X1, ImPlotScale_Time);
    ImPlot::SetNextFillStyle(IMPLOT_AUTO_COL, 0.5f);
    for (const auto &signal : signals) {
        if (!signal.data.empty()) {
            int offset = 0;
            if constexpr (requires { signal.offset; }) {
                offset = signal.offset;
            }
            ImPlot::PlotLine((signal.signalName).c_str(),
                    &signal.data[0].x,
                    &signal.data[0].y,
                    signal.data.size(),
                    0,
                    offset,
                    2 * sizeof(double));

            // Add tags with signal value
            DataPoint lastPoint = signal.data.back();
            ImVec4    col       = ImPlot::GetLastItemColor();
            ImPlot::TagY(lastPoint.y, col, "%2f", lastPoint.y);
        }
    }
}

void plotPowerSpectrum(std::vector<Buffer> &signals, std::vector<Buffer> &limitingCurve, const bool violation, ImFont *fontawesome) {
    static ImPlotAxisFlags xflags = ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_RangeFit;
    static ImPlotAxisFlags yflags = ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_RangeFit;
    ImPlot::SetupAxes("Frequency (Hz)", "Power Density (dB)", xflags, yflags);
    ImPlot::SetNextFillStyle(IMPLOT_AUTO_COL, 0.5f);
    plotSignals(signals);
    plotSignals(limitingCurve);
    if (violation) {
        addPlotNotice("error: pulsed power limits exceeded!", fontawesome, ICON_FA_TRIANGLE_EXCLAMATION, { 1.0, 0, 0, 1.0 }, { 0.15, 0.05 });
        addPlotNotice("warning: pulsed power limits exceeded!", fontawesome, ICON_FA_TRIANGLE_EXCLAMATION, { 1, 0.5, 0, 1.0 }, { 0.15, 0.25 });
        addPlotNotice("info: pulsed power limits exceeded!", fontawesome, ICON_FA_CIRCLE_QUESTION, { 0, 0, 1.0, 1.0 }, { 0.15, 0.45 });
    }
}

} // namespace Plotter
