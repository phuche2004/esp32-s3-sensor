#pragma once
#include <Arduino.h>

// Toan bo CSS Stylesheet cua Web Captive Portal luu trong bo nho Flash (PROGMEM)
static const char WEB_STYLES_CSS[] PROGMEM = R"rawliteral(
* { box-sizing: border-box; margin: 0; padding: 0; }
html, body {
    overscroll-behavior: none !important;
    overscroll-behavior-y: none !important;
}
body {
    font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, 'Inter', sans-serif;
    background: #080612;
    background-image: radial-gradient(circle at 50% 0%, #20133b 0%, #080612 85%);
    color: #f1f5f9;
    min-height: 100vh;
    display: flex;
    align-items: center;
    justify-content: center;
    padding: 14px 10px;
    overflow-x: hidden;
}
.container {
    width: 100%;
    max-width: 420px;
    background: rgba(18, 14, 34, 0.9);
    backdrop-filter: blur(18px);
    -webkit-backdrop-filter: blur(18px);
    border: 1px solid rgba(168, 85, 247, 0.25);
    padding: 14px 16px 18px 16px;
    border-radius: 22px;
    box-shadow: 0 25px 50px -12px rgba(0, 0, 0, 0.8), 0 0 35px rgba(139, 92, 246, 0.16);
    position: relative;
}
h2 {
    font-size: 20px;
    font-weight: 700;
    letter-spacing: -0.3px;
    background: linear-gradient(to right, #f8fafc, #c084fc);
    -webkit-background-clip: text;
    -webkit-text-fill-color: transparent;
    margin-bottom: 3px;
}

/* Realtime Sensor Widget */
.sensor-widget {
    background: rgba(13, 9, 28, 0.78);
    border: 1px solid rgba(168, 85, 247, 0.26);
    border-radius: 16px;
    padding: 12px 14px;
    margin-bottom: 18px;
    box-shadow: 0 4px 16px rgba(0, 0, 0, 0.4);
}
.widget-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 10px;
}
.live-tag {
    display: flex;
    align-items: center;
    gap: 6px;
    font-size: 11px;
    font-weight: 700;
    color: #c084fc;
    letter-spacing: 0.5px;
    text-transform: uppercase;
}
.live-dot {
    width: 7px;
    height: 7px;
    background: #10b981;
    border-radius: 50%;
    box-shadow: 0 0 8px #10b981;
    animation: pulseDot 1.4s infinite ease-in-out;
}
@keyframes pulseDot {
    0%, 100% { opacity: 1; transform: scale(1); }
    50% { opacity: 0.3; transform: scale(0.8); }
}
.sensor-badge-normal {
    font-size: 10px;
    font-weight: 700;
    color: #34d399;
    background: rgba(16, 185, 129, 0.14);
    border: 1px solid rgba(16, 185, 129, 0.35);
    padding: 2px 8px;
    border-radius: 999px;
    text-transform: uppercase;
}
.sensor-badge-alert {
    font-size: 10px;
    font-weight: 700;
    color: #f87171;
    background: rgba(239, 68, 68, 0.16);
    border: 1px solid rgba(239, 68, 68, 0.45);
    padding: 2px 8px;
    border-radius: 999px;
    text-transform: uppercase;
    animation: pulseDot 0.8s infinite;
}
.widget-cards {
    display: grid;
    grid-template-columns: repeat(3, 1fr);
    gap: 7px;
}
.sensor-data-card {
    background: #150f2f;
    border: 1px solid #2a1e52;
    border-radius: 12px;
    padding: 9px 8px;
    display: flex;
    flex-direction: column;
    gap: 2px;
}
.sensor-card-title {
    display: flex;
    align-items: center;
    gap: 5px;
    font-size: 10.5px;
    font-weight: 600;
    color: #94a3b8;
    white-space: nowrap;
}
.sensor-val-box {
    font-size: 18px;
    font-weight: 800;
    color: #f8fafc;
    line-height: 1.2;
    font-variant-numeric: tabular-nums;
}
.sensor-val-box small {
    font-size: 11px;
    font-weight: 600;
    color: #a78bfa;
    margin-left: 2px;
}

/* Stress Test Card Styles */
.stress-card {
    background: rgba(22, 17, 39, 0.9);
    border: 1px solid rgba(245, 158, 11, 0.35);
    border-radius: 16px;
    padding: 13px 14px;
    box-shadow: 0 4px 18px rgba(0, 0, 0, 0.45);
}
.stress-header {
    display: flex;
    align-items: center;
    justify-content: space-between;
    margin-bottom: 6px;
}
.stress-title {
    display: flex;
    align-items: center;
    gap: 6px;
    font-size: 11.5px;
    font-weight: 700;
    color: #fbbf24;
    letter-spacing: 0.03em;
}
.stress-cpu-badge {
    font-size: 11px;
    font-weight: 700;
    color: #38bdf8;
    background: rgba(56, 189, 248, 0.14);
    border: 1px solid rgba(56, 189, 248, 0.35);
    padding: 2px 8px;
    border-radius: 999px;
    font-variant-numeric: tabular-nums;
}
.stress-cpu-badge.active {
    color: #ef4444;
    background: rgba(239, 68, 68, 0.16);
    border-color: rgba(239, 68, 68, 0.4);
}
.stress-temp-badge {
    font-size: 12px;
    font-weight: 800;
    color: #f59e0b;
    background: rgba(245, 158, 11, 0.15);
    border: 1px solid rgba(245, 158, 11, 0.35);
    padding: 2px 8px;
    border-radius: 999px;
    font-variant-numeric: tabular-nums;
}
.stress-desc {
    font-size: 11px;
    color: #94a3b8;
    line-height: 1.35;
    margin-bottom: 10px;
}
.stress-timer-box {
    display: flex;
    align-items: center;
    justify-content: space-between;
    background: rgba(13, 9, 26, 0.85);
    border: 1px solid rgba(245, 158, 11, 0.25);
    border-radius: 10px;
    padding: 7px 12px;
    font-size: 12px;
    color: #cbd5e1;
    margin-bottom: 10px;
}
.stress-timer-box strong#stressTimerVal {
    font-family: 'SF Mono', Consolas, monospace;
    font-size: 17px;
    font-weight: 800;
    color: #ef4444;
    letter-spacing: 0.05em;
    font-variant-numeric: tabular-nums;
}
.btn-stress-start {
    width: 100%;
    padding: 10px;
    background: linear-gradient(135deg, #d97706, #b45309);
    border: none;
    border-radius: 10px;
    color: #fff;
    font-size: 12px;
    font-weight: 700;
    cursor: pointer;
    box-shadow: 0 4px 12px rgba(217, 119, 6, 0.3);
    transition: all 0.2s ease;
}
.btn-stress-start:hover {
    background: linear-gradient(135deg, #f59e0b, #d97706);
}
.btn-stress-stop {
    width: 100%;
    padding: 10px;
    background: linear-gradient(135deg, #dc2626, #b91c1c);
    border: none;
    border-radius: 10px;
    color: #fff;
    font-size: 12px;
    font-weight: 700;
    cursor: pointer;
    box-shadow: 0 4px 14px rgba(220, 38, 38, 0.4);
    transition: all 0.2s ease;
}

.alert-box {
    padding: 12px 16px;
    border-radius: 12px;
    font-size: 13px;
    line-height: 1.5;
    margin-bottom: 18px;
    display: none;
    animation: fadeIn 0.3s ease;
}
.alert-error {
    background: rgba(239, 68, 68, 0.12);
    border: 1px solid rgba(239, 68, 68, 0.4);
    color: #fca5a5;
}
.alert-success {
    background: rgba(16, 185, 129, 0.12);
    border: 1px solid rgba(16, 185, 129, 0.4);
    color: #6ee7b7;
}

label {
    display: block;
    font-size: 12px;
    font-weight: 600;
    text-transform: uppercase;
    letter-spacing: 0.5px;
    color: #a78bfa;
    margin: 15px 0 6px 0;
}

.input-group {
    display: flex;
    gap: 8px;
    align-items: stretch;
    width: 100%;
}
.input-group input {
    flex: 1;
    min-width: 0;
}

input[type='text'], input[type='password'] {
    width: 100%;
    padding: 12px 14px;
    background: #0c0919;
    border: 1px solid #291f47;
    border-radius: 12px;
    color: #fff;
    font-size: 14px;
    outline: none;
    transition: all 0.25s ease;
}
input[type='text']:focus, input[type='password']:focus {
    border-color: #a855f7;
    box-shadow: 0 0 0 3px rgba(168, 85, 247, 0.2);
    background: #100c22;
}
input::placeholder { color: #64748b; font-size: 13px; }

.btn-open-modal {
    background: linear-gradient(135deg, rgba(168, 85, 247, 0.2), rgba(139, 92, 246, 0.08));
    border: 1px solid rgba(168, 85, 247, 0.45);
    color: #d8b4fe;
    padding: 0 14px;
    border-radius: 12px;
    font-size: 13px;
    font-weight: 600;
    cursor: pointer;
    display: inline-flex;
    align-items: center;
    gap: 6px;
    white-space: nowrap;
    transition: all 0.2s ease;
    flex-shrink: 0;
}
.btn-open-modal:hover {
    background: rgba(168, 85, 247, 0.32);
    border-color: #c084fc;
    color: #ffffff;
    box-shadow: 0 0 14px rgba(168, 85, 247, 0.3);
}
.btn-open-modal:active {
    transform: scale(0.97);
}

.toggle-box {
    display: flex;
    align-items: center;
    justify-content: space-between;
    background: #130f29;
    border: 1px solid #291f47;
    padding: 12px 16px;
    border-radius: 12px;
    margin-top: 16px;
}
.toggle-box span { font-size: 13px; font-weight: 600; color: #e2e8f0; }
.switch { position: relative; display: inline-block; width: 44px; height: 24px; flex-shrink: 0; }
.switch input { opacity: 0; width: 0; height: 0; }
.slider {
    position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0;
    background-color: #332757; transition: .3s; border-radius: 24px;
}
.slider:before {
    position: absolute; content: ""; height: 18px; width: 18px; left: 3px; bottom: 3px;
    background-color: white; transition: .3s; border-radius: 50%;
}
input:checked + .slider { background: #8b5cf6; }
input:checked + .slider:before { transform: translateX(20px); }

.btn-submit {
    width: 100%;
    padding: 14px;
    background: linear-gradient(135deg, #8b5cf6 0%, #6d28d9 100%);
    color: #fff;
    font-size: 15px;
    font-weight: 700;
    border: none;
    border-radius: 14px;
    cursor: pointer;
    margin-top: 24px;
    transition: all 0.25s;
    display: flex;
    align-items: center;
    justify-content: center;
    box-shadow: 0 10px 20px -5px rgba(109, 40, 217, 0.45);
}
.btn-submit:hover:not(:disabled) {
    background: linear-gradient(135deg, #9333ea 0%, #7c3aed 100%);
    box-shadow: 0 12px 25px -4px rgba(147, 51, 234, 0.6);
    transform: translateY(-1px);
}
.btn-submit:disabled { opacity: 0.7; cursor: not-allowed; transform: none; }

.spinner-sm {
    display: inline-block;
    width: 16px;
    height: 16px;
    border: 2px solid rgba(255,255,255,0.3);
    border-top-color: #fff;
    border-radius: 50%;
    animation: spin 0.8s infinite linear;
    margin-right: 8px;
}
.rotate-anim { animation: spin 0.8s infinite linear; }
@keyframes spin { to { transform: rotate(360deg); } }
@keyframes fadeIn { from { opacity: 0; transform: translateY(-6px); } to { opacity: 1; transform: translateY(0); } }
@keyframes zoomIn { from { opacity: 0; transform: scale(0.92); } to { opacity: 1; transform: scale(1); } }

/* Modal Styles */
.modal-overlay {
    position: fixed;
    top: 0; left: 0; right: 0; bottom: 0;
    background: rgba(4, 2, 12, 0.8);
    backdrop-filter: blur(10px);
    -webkit-backdrop-filter: blur(10px);
    z-index: 1000;
    display: none;
    align-items: center;
    justify-content: center;
    padding: 16px;
}
.modal-box {
    background: #120e26;
    border: 1px solid rgba(168, 85, 247, 0.35);
    border-radius: 20px;
    width: 100%;
    max-width: 390px;
    max-height: 84vh;
    display: flex;
    flex-direction: column;
    box-shadow: 0 25px 60px rgba(0,0,0,0.9), 0 0 40px rgba(139, 92, 246, 0.2);
    animation: zoomIn 0.22s cubic-bezier(0.16, 1, 0.3, 1);
    overflow: hidden;
}
.modal-header {
    display: flex;
    align-items: center;
    justify-content: space-between;
    padding: 16px 18px;
    border-bottom: 1px solid #241a45;
    background: #16112f;
}
.modal-title {
    display: flex;
    align-items: center;
    gap: 8px;
    font-weight: 700;
    font-size: 15px;
    color: #f1f5f9;
}
.modal-close-btn {
    background: transparent;
    border: none;
    color: #94a3b8;
    font-size: 24px;
    line-height: 1;
    cursor: pointer;
    width: 32px;
    height: 32px;
    border-radius: 8px;
    display: flex;
    align-items: center;
    justify-content: center;
    transition: all 0.15s;
}
.modal-close-btn:hover {
    color: #fff;
    background: rgba(255, 255, 255, 0.1);
}
.modal-subbar {
    display: flex;
    align-items: center;
    justify-content: space-between;
    padding: 10px 18px;
    background: #0d091d;
    border-bottom: 1px solid #1f173b;
}
.modal-status-text {
    font-size: 12px;
    color: #94a3b8;
}
.btn-refresh {
    background: #201542;
    border: 1px solid #4d3385;
    color: #c084fc;
    padding: 5px 12px;
    border-radius: 999px;
    font-size: 11px;
    font-weight: 600;
    cursor: pointer;
    display: inline-flex;
    align-items: center;
    gap: 5px;
    transition: all 0.2s;
}
.btn-refresh:hover {
    background: #2d1c5e;
    border-color: #a855f7;
    color: #fff;
}
.modal-wifi-list {
    padding: 12px;
    overflow-y: auto;
    max-height: 320px;
    display: flex;
    flex-direction: column;
    gap: 8px;
}
.modal-wifi-list::-webkit-scrollbar { width: 5px; }
.modal-wifi-list::-webkit-scrollbar-thumb { background: #3d2c6b; border-radius: 4px; }

.wifi-card {
    background: #171131;
    border: 1px solid #291d4e;
    padding: 11px 14px;
    border-radius: 12px;
    cursor: pointer;
    display: flex;
    align-items: center;
    justify-content: space-between;
    transition: all 0.18s ease;
}
.wifi-card:hover {
    background: #261a50;
    border-color: #a855f7;
    transform: translateY(-1px);
    box-shadow: 0 4px 14px rgba(139, 92, 246, 0.22);
}
.wifi-card:active {
    transform: scale(0.98);
}
.wifi-card-left {
    display: flex;
    align-items: center;
    gap: 12px;
    overflow: hidden;
}
.wifi-name {
    font-size: 14px;
    font-weight: 600;
    color: #f8fafc;
    white-space: nowrap;
    overflow: hidden;
    text-overflow: ellipsis;
    max-width: 200px;
}
.signal-badge {
    font-size: 11px;
    font-weight: 600;
    padding: 3px 8px;
    border-radius: 999px;
    background: rgba(168, 85, 247, 0.15);
    color: #c084fc;
    border: 1px solid rgba(168, 85, 247, 0.25);
    white-space: nowrap;
}
.signal-bars {
    display: flex;
    align-items: flex-end;
    gap: 2.5px;
    height: 14px;
    width: 14px;
    flex-shrink: 0;
}
.sig-bar {
    width: 2.5px;
    background: #372a5a;
    border-radius: 1px;
}
.sig-bar.b1 { height: 25%; }
.sig-bar.b2 { height: 50%; }
.sig-bar.b3 { height: 75%; }
.sig-bar.b4 { height: 100%; }
.sig-bar.on {
    background: #c084fc;
    box-shadow: 0 0 5px rgba(192, 132, 252, 0.7);
}

.modal-footer {
    padding: 12px 18px;
    border-top: 1px solid #241a45;
    background: #16112f;
    display: flex;
    justify-content: flex-end;
}
.btn-modal-close {
    background: #20173d;
    border: 1px solid #3d2b6b;
    color: #cbd5e1;
    padding: 8px 18px;
    border-radius: 10px;
    font-size: 13px;
    font-weight: 600;
    cursor: pointer;
    transition: all 0.2s;
}
.btn-modal-close:hover {
    background: #2b204e;
    color: #fff;
}

/* Success Card Styling */
.success-badge {
    width: 64px;
    height: 64px;
    border-radius: 50%;
    background: rgba(16, 185, 129, 0.15);
    border: 2px solid #10b981;
    display: flex;
    align-items: center;
    justify-content: center;
    margin: 8px auto 16px auto;
    box-shadow: 0 0 25px rgba(16, 185, 129, 0.3);
}
.success-info {
    background: #0c0919;
    border: 1px solid #291f47;
    border-radius: 14px;
    padding: 16px;
    margin: 16px 0;
    text-align: left;
}
.success-row {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 8px 0;
}
.success-row:not(:last-child) { border-bottom: 1px solid #1a1435; }
.countdown-card {
    background: linear-gradient(145deg, #181135, #0f0a22);
    border-radius: 16px;
    padding: 20px 16px;
    border: 1px dashed rgba(168, 85, 247, 0.35);
}
.countdown-number {
    font-size: 42px;
    font-weight: 800;
    color: #c084fc;
    line-height: 1.1;
    margin: 6px 0 12px 0;
    text-shadow: 0 0 20px rgba(192, 132, 252, 0.4);
}
.progress-bar-bg {
    height: 6px;
    background: #251c44;
    border-radius: 999px;
    overflow: hidden;
    margin-bottom: 12px;
}
.progress-bar-fill {
    height: 100%;
    width: 100%;
    background: linear-gradient(to right, #8b5cf6, #c084fc);
    transition: width 1s linear;
}

/* Tab Navigation Styles */
.tab-nav {
    display: flex;
    background: rgba(15, 12, 28, 0.85);
    border: 1px solid rgba(168, 85, 247, 0.25);
    border-radius: 14px;
    padding: 4px;
    margin-bottom: 18px;
    gap: 3px;
    overflow-x: auto;
    scrollbar-width: none;
}
.tab-nav::-webkit-scrollbar { display: none; }
.tab-btn {
    flex: 1;
    min-width: 0;
    display: flex;
    align-items: center;
    justify-content: center;
    gap: 4px;
    padding: 8px 3px;
    background: transparent;
    border: none;
    color: #94a3b8;
    font-size: 11px;
    font-weight: 600;
    border-radius: 10px;
    cursor: pointer;
    transition: all 0.2s ease;
    white-space: nowrap;
}
.tab-btn.active {
    background: linear-gradient(135deg, rgba(168, 85, 247, 0.35), rgba(139, 92, 246, 0.25));
    color: #f1f5f9;
    border: 1px solid rgba(168, 85, 247, 0.45);
    box-shadow: 0 4px 12px rgba(139, 92, 246, 0.2);
}

/* Sensor Settings Styles */
.sensor-cfg-grid {
    display: grid;
    grid-template-columns: 1fr;
    gap: 11px;
    margin-bottom: 14px;
}
.sensor-cfg-card {
    background: rgba(26, 20, 48, 0.65);
    border: 1px solid rgba(168, 85, 247, 0.2);
    border-radius: 14px;
    padding: 12px 14px;
    display: flex;
    flex-direction: column;
    gap: 5px;
}
.sensor-cfg-header {
    display: flex;
    align-items: center;
    justify-content: space-between;
}
.sensor-cfg-title {
    font-size: 13px;
    font-weight: 700;
    color: #f8fafc;
}
.sensor-cfg-desc {
    font-size: 11.5px;
    color: #94a3b8;
    line-height: 1.35;
    margin-bottom: 3px;
}
.sensor-input-wrap {
    display: flex;
    align-items: center;
    gap: 8px;
    background: #0c0919;
    border: 1px solid #291f47;
    border-radius: 10px;
    padding: 8px 12px;
    transition: border-color 0.2s;
}
.sensor-input-wrap:focus-within {
    border-color: #a855f7;
    box-shadow: 0 0 0 3px rgba(168, 85, 247, 0.2);
}
.sensor-num-input {
    flex: 1;
    background: transparent;
    border: none;
    outline: none;
    color: #fff;
    font-size: 15px;
    font-weight: 700;
    font-family: 'SF Mono', Consolas, monospace;
}
.sensor-unit-badge {
    font-size: 12px;
    font-weight: 700;
    color: #c084fc;
}
.btn-action-row {
    display: flex;
    gap: 8px;
    margin-top: 14px;
}
.btn-secondary {
    background: rgba(30, 22, 56, 0.85);
    border: 1px solid rgba(168, 85, 247, 0.3);
    color: #cbd5e1;
    font-size: 13px;
    font-weight: 600;
    padding: 12px 14px;
    border-radius: 12px;
    cursor: pointer;
    transition: all 0.2s;
    white-space: nowrap;
    display: flex;
    align-items: center;
    justify-content: center;
}
.btn-secondary:hover {
    background: rgba(48, 35, 90, 0.95);
    border-color: #a855f7;
    color: #fff;
}

/* LED Studio Styles */
.led-section-title {
    font-size: 11px;
    font-weight: 700;
    color: #94a3b8;
    letter-spacing: 0.08em;
    text-transform: uppercase;
    margin-bottom: 10px;
}
.led-modes-grid {
    display: grid;
    grid-template-columns: repeat(2, 1fr);
    gap: 9px;
    margin-bottom: 14px;
}
.led-mode-card {
    background: rgba(26, 20, 48, 0.7);
    border: 1px solid rgba(168, 85, 247, 0.2);
    border-radius: 14px;
    padding: 11px 10px;
    cursor: pointer;
    transition: all 0.2s ease;
    position: relative;
    user-select: none;
}
.led-mode-card:hover {
    border-color: rgba(168, 85, 247, 0.5);
    background: rgba(35, 26, 64, 0.85);
}
.led-mode-card.active {
    border-color: #a855f7;
    background: linear-gradient(135deg, rgba(168, 85, 247, 0.3), rgba(139, 92, 246, 0.15));
    box-shadow: 0 0 14px rgba(168, 85, 247, 0.35);
}
.mode-header {
    display: flex;
    align-items: center;
    justify-content: space-between;
    margin-bottom: 5px;
}
.mode-name {
    font-size: 13px;
    font-weight: 700;
    color: #f8fafc;
}
.mode-desc {
    font-size: 10.5px;
    color: #94a3b8;
    line-height: 1.3;
}
.mode-badge-hot {
    font-size: 9px;
    font-weight: 800;
    background: #ef4444;
    color: #fff;
    padding: 2px 5px;
    border-radius: 5px;
    letter-spacing: 0.04em;
}
.mode-badge-def {
    font-size: 9px;
    font-weight: 800;
    background: #a855f7;
    color: #fff;
    padding: 2px 5px;
    border-radius: 5px;
    letter-spacing: 0.04em;
}
.led-control-box {
    background: rgba(26, 20, 48, 0.6);
    border: 1px solid rgba(168, 85, 247, 0.18);
    border-radius: 14px;
    padding: 12px 13px;
    margin-bottom: 11px;
}
.led-box-label {
    display: flex;
    justify-content: space-between;
    align-items: center;
    font-size: 11px;
    font-weight: 700;
    color: #94a3b8;
    margin-bottom: 8px;
}

/* Inline Color Wheel Panel (Tab LED) */
.color-inline-panel {
    background: rgba(18, 14, 34, 0.85);
    border: 1px solid rgba(168, 85, 247, 0.28);
    border-radius: 18px;
    padding: 14px 14px 16px 14px;
    margin-bottom: 12px;
    box-shadow: 0 4px 20px rgba(0, 0, 0, 0.4);
    touch-action: pan-y !important;
}
.color-preview-pill {
    display: inline-flex;
    align-items: center;
    gap: 7px;
    background: #0d091d;
    border: 1px solid rgba(168, 85, 247, 0.35);
    padding: 3px 9px;
    border-radius: 999px;
}
.color-preview-dot {
    width: 12px;
    height: 12px;
    border-radius: 50%;
    background: #a855f7;
    box-shadow: 0 0 6px #a855f7;
}
.color-wheel-wrapper {
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    width: 100%;
    padding: 0;
    touch-action: pan-y !important;
    -webkit-user-select: none;
    user-select: none;
}
.color-wheel-canvas-container {
    position: relative;
    width: 260px;
    height: 260px;
    border-radius: 50%;
    user-select: none;
    touch-action: pan-y !important;
    -webkit-touch-callout: none;
    -webkit-user-drag: none;
    margin: 12px auto 12px auto;
}
#colorWheelCanvas {
    width: 260px;
    height: 260px;
    border-radius: 50%;
    box-shadow: 0 6px 26px rgba(0, 0, 0, 0.7), 0 0 20px rgba(168, 85, 247, 0.4);
    cursor: crosshair;
    display: block;
    touch-action: pan-y !important;
    -webkit-user-select: none;
    user-select: none;
    -webkit-user-drag: none;
}
#wheelHandle {
    position: absolute;
    top: 0;
    left: 0;
    width: 28px;
    height: 28px;
    border-radius: 50%;
    border: 3px solid #ffffff;
    box-shadow: 0 0 10px rgba(0,0,0,0.85), inset 0 0 4px rgba(0,0,0,0.35);
    transform: translate3d(116px, 116px, 0);
    pointer-events: none;
    background: #a855f7;
    touch-action: none !important;
    will-change: transform;
}
.color-preview-bar {
    display: flex;
    align-items: center;
    justify-content: space-between;
    width: 100%;
    margin-top: 6px;
    margin-bottom: 12px;
    background: rgba(15, 12, 28, 0.7);
    padding: 8px 14px;
    border-radius: 12px;
    border: 1px solid rgba(168, 85, 247, 0.25);
}
.color-preview-left {
    display: flex;
    align-items: center;
    gap: 9px;
}
.color-preview-badge {
    width: 26px;
    height: 26px;
    border-radius: 7px;
    border: 2px solid rgba(255, 255, 255, 0.35);
    background: #a855f7;
    box-shadow: 0 0 8px rgba(168, 85, 247, 0.4);
}
.color-hex-text {
    font-family: 'SF Mono', Consolas, monospace;
    font-size: 13px;
    font-weight: 700;
    color: #f1f5f9;
    letter-spacing: 0.05em;
}

.color-chips {
    display: flex;
    flex-wrap: wrap;
    gap: 7px;
    justify-content: center;
    width: 100%;
    margin-top: 4px;
}
.color-chip {
    width: 26px;
    height: 26px;
    border-radius: 50%;
    border: 2px solid rgba(255, 255, 255, 0.22);
    cursor: pointer;
    transition: transform 0.15s ease, border-color 0.15s ease;
}
.color-chip:hover {
    transform: scale(1.2);
    border-color: #fff;
    box-shadow: 0 0 10px rgba(255,255,255,0.4);
}
.led-slider {
    width: 100%;
    height: 6px;
    background: #251c44;
    border-radius: 999px;
    outline: none;
    -webkit-appearance: none;
}
.led-slider::-webkit-slider-thumb {
    -webkit-appearance: none;
    width: 18px;
    height: 18px;
    border-radius: 50%;
    background: #c084fc;
    border: 2px solid #fff;
    cursor: pointer;
    box-shadow: 0 0 10px rgba(168, 85, 247, 0.5);
}
.led-toast {
    display: none;
    text-align: center;
    padding: 9px;
    margin-top: 10px;
    border-radius: 10px;
    background: rgba(34, 197, 94, 0.2);
    border: 1px solid rgba(34, 197, 94, 0.4);
    color: #4ade80;
    font-size: 12px;
    font-weight: 600;
}

/* Tab 4: Biểu Đồ Lịch Sử Đo */
.chart-header-row {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 12px;
}
.chart-filter-group {
    display: flex;
    gap: 4px;
    background: rgba(13, 9, 26, 0.8);
    padding: 3px;
    border-radius: 999px;
    border: 1px solid rgba(255, 255, 255, 0.1);
}
.btn-filter {
    background: transparent;
    border: none;
    color: #94a3b8;
    padding: 5px 12px;
    border-radius: 999px;
    font-size: 11px;
    font-weight: 700;
    cursor: pointer;
    transition: all 0.2s ease;
}
.btn-filter.active {
    background: #38bdf8;
    color: #020617;
    box-shadow: 0 2px 8px rgba(56, 189, 248, 0.4);
}
.btn-export-csv {
    display: flex;
    align-items: center;
    gap: 5px;
    background: rgba(56, 189, 248, 0.12);
    border: 1px solid rgba(56, 189, 248, 0.35);
    color: #38bdf8;
    padding: 6px 12px;
    border-radius: 10px;
    font-size: 11.5px;
    font-weight: 700;
    cursor: pointer;
    transition: all 0.2s ease;
}
.btn-export-csv:hover {
    background: rgba(56, 189, 248, 0.25);
}
.chart-summary-cards {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 8px;
    margin-bottom: 12px;
}
.chart-stat-card {
    background: rgba(13, 9, 26, 0.7);
    border: 1px solid rgba(255, 255, 255, 0.08);
    border-radius: 12px;
    padding: 10px 12px;
}
.stat-label {
    font-size: 11px;
    font-weight: 700;
    text-transform: uppercase;
    letter-spacing: 0.03em;
    margin-bottom: 4px;
}
.stat-val {
    font-family: 'SF Mono', Consolas, monospace;
    font-size: 14px;
    font-weight: 800;
    color: #f8fafc;
}
.stat-val small {
    font-size: 11px;
    color: #94a3b8;
}
.chart-legend-row {
    display: flex;
    align-items: center;
    justify-content: space-between;
    margin-bottom: 8px;
    padding: 0 4px;
}
.chart-legend-item {
    display: flex;
    align-items: center;
    gap: 6px;
    font-size: 11px;
    color: #cbd5e1;
}
.legend-dot {
    width: 9px;
    height: 9px;
    border-radius: 50%;
}
.chart-count-tag {
    font-size: 11px;
    color: #94a3b8;
    background: rgba(255, 255, 255, 0.06);
    padding: 2px 8px;
    border-radius: 6px;
}
.chart-wrapper {
    position: relative;
    width: 100%;
    background: rgba(10, 6, 22, 0.85);
    border: 1px solid rgba(255, 255, 255, 0.1);
    border-radius: 14px;
    overflow: hidden;
    padding: 10px 4px 6px 4px;
    box-sizing: border-box;
}
#historyCanvas {
    display: block;
    width: 100%;
    height: 210px;
    cursor: crosshair;
}
.chart-tooltip {
    position: absolute;
    pointer-events: none;
    background: rgba(15, 23, 42, 0.95);
    border: 1px solid rgba(56, 189, 248, 0.4);
    box-shadow: 0 4px 16px rgba(0, 0, 0, 0.6);
    border-radius: 8px;
    padding: 6px 10px;
    font-size: 11px;
    color: #f8fafc;
    z-index: 10;
    white-space: nowrap;
    transition: opacity 0.15s ease;
}

/* Tab 2: Dashboard Thông Số Cảm Biến & Hệ Thống */
.sensor-sec-header {
    display: flex;
    align-items: center;
    gap: 8px;
    margin-bottom: 8px;
}
.sensor-sec-pill {
    font-size: 10.5px;
    font-weight: 800;
    padding: 2px 8px;
    border-radius: 999px;
    letter-spacing: 0.04em;
}
.sensor-sec-text {
    font-size: 11px;
    font-weight: 700;
    color: #94a3b8;
    letter-spacing: 0.03em;
}
.sensor-dash-grid {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 8px;
    margin-bottom: 4px;
}
.sensor-stat-box {
    background: rgba(13, 9, 26, 0.75);
    border: 1px solid rgba(255, 255, 255, 0.08);
    border-radius: 12px;
    padding: 9px 11px;
    display: flex;
    flex-direction: column;
    justify-content: space-between;
}
.stat-box-top {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 3px;
}
.stat-box-title {
    font-size: 10.5px;
    font-weight: 700;
    color: #cbd5e1;
}
.stat-box-unit {
    font-size: 10.5px;
    font-weight: 800;
    font-variant-numeric: tabular-nums;
}
.stat-box-num {
    font-family: 'SF Mono', Consolas, monospace;
    font-size: 17px;
    font-weight: 800;
    color: #f8fafc;
    letter-spacing: 0.02em;
}
.stat-box-num small {
    font-size: 11px;
    color: #94a3b8;
    font-weight: 600;
}
.stat-box-sub {
    font-size: 10px;
    color: #64748b;
    margin-top: 3px;
    white-space: nowrap;
    overflow: hidden;
    text-overflow: ellipsis;
}

/* Responsive Breakpoints */
@media (max-width: 480px) {
    body { padding: 12px 10px; }
    .container { padding: 20px 15px; border-radius: 20px; }
    h2 { font-size: 20px; }
    p.subtitle { font-size: 12px; }
    .logo-icon { width: 44px; height: 44px; margin-bottom: 8px; }
    input[type='text'], input[type='password'] { padding: 11px 12px; font-size: 14px; }
    .btn-open-modal { padding: 0 11px; font-size: 12px; }
    .btn-submit { padding: 13px; font-size: 14px; margin-top: 20px; }
    .modal-box { max-width: 95vw; max-height: 86vh; border-radius: 18px; }
    .modal-wifi-list { max-height: 270px; padding: 10px; }
    .wifi-name { max-width: 165px; font-size: 13px; }
    .sensor-val-box { font-size: 19px; }
    .led-modes-grid { gap: 7px; }
    .led-mode-card { padding: 9px 8px; }
}
.btn-reset-wifi {
    display: block;
    width: 100%;
    margin-top: 10px;
    padding: 11px 16px;
    background: rgba(239, 68, 68, 0.12);
    border: 1px solid rgba(239, 68, 68, 0.45);
    border-radius: 10px;
    color: #fca5a5;
    font-size: 13px;
    font-weight: 600;
    letter-spacing: 0.3px;
    cursor: pointer;
    transition: background 0.2s, border-color 0.2s, color 0.2s;
    text-align: center;
}
.btn-reset-wifi:hover, .btn-reset-wifi:active {
    background: rgba(239, 68, 68, 0.24);
    border-color: #ef4444;
    color: #fff;
}
)rawliteral";
