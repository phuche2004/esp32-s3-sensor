#pragma once
#include <Arduino.h>

// Toan bo Client-side JavaScript cua Web Captive Portal luu trong bo nho Flash (PROGMEM)
static const char WEB_SCRIPT_JS[] PROGMEM = R"rawliteral(
var hasScanned = false;

function openWifiModal() {
    var modal = document.getElementById('wifiModal');
    modal.style.display = 'flex';
    if (!hasScanned) {
        scanWiFi();
    }
}

function closeWifiModal() {
    var modal = document.getElementById('wifiModal');
    modal.style.display = 'none';
}

function handleModalOverlayClick(e) {
    if (e.target.id === 'wifiModal') {
        closeWifiModal();
    }
}


function toggleEnterprise() {
    var isEnt = document.getElementById('is_ent').checked;
    document.getElementById('userBlock').style.display = isEnt ? 'block' : 'none';
}

function selectSSID(name) {
    document.getElementById('ssid').value = name;
    closeWifiModal();
    var pwd = document.getElementById('password');
    if (pwd) pwd.focus();
}

function renderSignalBars(rssi) {
    var count = 1;
    if (rssi >= -60) count = 4;
    else if (rssi >= -70) count = 3;
    else if (rssi >= -80) count = 2;

    var html = '<div class="signal-bars">';
    html += '<div class="sig-bar b1 ' + (count >= 1 ? 'on' : '') + '"></div>';
    html += '<div class="sig-bar b2 ' + (count >= 2 ? 'on' : '') + '"></div>';
    html += '<div class="sig-bar b3 ' + (count >= 3 ? 'on' : '') + '"></div>';
    html += '<div class="sig-bar b4 ' + (count >= 4 ? 'on' : '') + '"></div>';
    html += '</div>';
    return html;
}

function scanWiFi() {
    var list = document.getElementById('modalWifiList');
    var status = document.getElementById('modalStatus');
    var refreshSvg = document.getElementById('refreshSvg');
    var btnRefresh = document.getElementById('btnRefresh');

    refreshSvg.classList.add('rotate-anim');
    btnRefresh.disabled = true;
    status.innerText = 'Đang quét sóng Wi-Fi...';

    fetch('/scan')
        .then(r => r.json())
        .then(data => {
            refreshSvg.classList.remove('rotate-anim');
            btnRefresh.disabled = false;
            hasScanned = true;

            if (!data || data.length === 0) {
                list.innerHTML = '<div style="color:#64748b; text-align:center; padding:24px 0; font-size:13px;">Không tìm thấy mạng Wi-Fi nào</div>';
                status.innerText = 'Tìm thấy 0 mạng';
                return;
            }

            var map = {};
            data.forEach(item => {
                var s = item.ssid ? item.ssid.trim() : '';
                if (s.length > 0) {
                    if (!map[s] || item.rssi > map[s].rssi) {
                        map[s] = item;
                    }
                }
            });

            var uniqueList = Object.values(map);
            uniqueList.sort((a, b) => b.rssi - a.rssi);

            status.innerText = 'Tìm thấy ' + uniqueList.length + ' mạng khả dụng';

            var html = '';
            uniqueList.forEach(item => {
                var safeName = item.ssid.replace(/\\/g, '\\\\').replace(/'/g, "\\'");
                html += '<div class="wifi-card" onclick="selectSSID(\'' + safeName + '\')">';
                html += '  <div class="wifi-card-left">';
                html += renderSignalBars(item.rssi);
                html += '    <span class="wifi-name">' + item.ssid + '</span>';
                html += '  </div>';
                html += '  <span class="signal-badge">' + item.rssi + ' dBm</span>';
                html += '</div>';
            });
            list.innerHTML = html;
        })
        .catch(err => {
            refreshSvg.classList.remove('rotate-anim');
            btnRefresh.disabled = false;
            status.innerText = 'Lỗi quét mạng';
            list.innerHTML = '<div style="color:#f87171; text-align:center; padding:20px 0; font-size:13px;">Không thể quét sóng Wi-Fi. Vui lòng bấm làm mới.</div>';
        });
}

// Real-time Sensor Data Polling
var fetchingSensor = false;
function fetchSensorData() {
    if (fetchingSensor) return;
    fetchingSensor = true;
    fetch('/sensor')
        .then(function(r) { return r.json(); })
        .then(function(data) {
            fetchingSensor = false;
            if (data) {
                if (data.valid) {
                    var tEl = document.getElementById('liveTemp');
                    var hEl = document.getElementById('liveHum');
                    var bEl = document.getElementById('sensorStatusBadge');
                    var stEl = document.getElementById('successTemp');
                    var shEl = document.getElementById('successHum');

                    var tStr = data.temp.toFixed(1);
                    var hStr = data.hum.toFixed(1);

                    if (tEl) tEl.innerText = tStr;
                    if (hEl) hEl.innerText = hStr;
                    if (stEl) stEl.innerText = tStr + ' °C';
                    if (shEl) shEl.innerText = hStr + ' %';

                    if (bEl) {
                        if (data.alert) {
                            bEl.className = 'sensor-badge-alert';
                            bEl.innerText = 'CẢNH BÁO';
                        } else {
                            bEl.className = 'sensor-badge-normal';
                            bEl.innerText = 'BÌNH THƯỜNG';
                        }
                    }
                }

                // Cập nhật nhiệt độ lõi chip nội bộ (ESP32-S3 Internal Sensor)
                if (data.chipTemp !== undefined) {
                    var cEl = document.getElementById('liveChipTemp');
                    if (cEl) cEl.innerText = data.chipTemp.toFixed(1);
                    var sbEl = document.getElementById('stressChipBadge');
                    if (sbEl) sbEl.innerText = data.chipTemp.toFixed(1) + ' °C';
                }

                // Cập nhật % CPU sử dụng thực tế
                if (data.cpu !== undefined) {
                    var cpuEl = document.getElementById('liveCpuLoad');
                    if (cpuEl) cpuEl.innerText = data.cpu.toFixed(1);
                    var scbEl = document.getElementById('stressCpuBadge');
                    if (scbEl) {
                        scbEl.innerText = 'CPU: ' + Math.round(data.cpu) + '%';
                        if (data.stressActive || data.cpu >= 90) {
                            scbEl.classList.add('active');
                        } else {
                            scbEl.classList.remove('active');
                        }
                    }
                }

                if (data.cpu0 !== undefined) {
                    var c0El = document.getElementById('stressCpu0Val');
                    if (c0El) c0El.innerText = Math.round(data.cpu0) + '%';
                }
                if (data.cpu1 !== undefined) {
                    var c1El = document.getElementById('stressCpu1Val');
                    if (c1El) c1El.innerText = Math.round(data.cpu1) + '%';
                }

                // Cập nhật Tab 2: Dashboard Thông Số Cảm Biến & Hệ Thống
                var dTemp = document.getElementById('dashTemp');
                var dHum = document.getElementById('dashHum');
                var dTempSt = document.getElementById('dashTempStatus');
                var dHumSt = document.getElementById('dashHumStatus');
                var dDew = document.getElementById('dashDewPoint');
                var dVpd = document.getElementById('dashVpd');

                if (data.valid) {
                    if (dTemp) dTemp.innerText = data.temp.toFixed(1);
                    if (dHum) dHum.innerText = data.hum.toFixed(1);
                    if (dTempSt) dTempSt.innerText = data.alert ? 'CẢNH BÁO' : 'Bình thường';
                    if (dHumSt) dHumSt.innerText = data.alert ? 'CẢNH BÁO' : 'Bình thường';
                    if (dDew && data.dewPoint !== undefined) dDew.innerText = data.dewPoint.toFixed(1);
                    if (dVpd && data.vpd !== undefined) dVpd.innerText = data.vpd.toFixed(2);
                }

                var dChip = document.getElementById('dashChipTemp');
                var dChipB = document.getElementById('dashChipBadge');
                if (data.chipTemp !== undefined) {
                    if (dChip) dChip.innerText = data.chipTemp.toFixed(1);
                    if (dChipB) dChipB.innerText = data.chipTemp.toFixed(1) + ' °C';
                }

                var dCpu = document.getElementById('dashCpuLoad');
                var dCpuB = document.getElementById('dashCpuBadge');
                var dCpu0 = document.getElementById('dashCpu0');
                var dCpu1 = document.getElementById('dashCpu1');
                if (data.cpu !== undefined) {
                    if (dCpu) dCpu.innerText = data.cpu.toFixed(1);
                    if (dCpuB) dCpuB.innerText = Math.round(data.cpu) + '%';
                }
                if (data.cpu0 !== undefined && dCpu0) dCpu0.innerText = Math.round(data.cpu0) + '%';
                if (data.cpu1 !== undefined && dCpu1) dCpu1.innerText = Math.round(data.cpu1) + '%';

                var dUsed = document.getElementById('dashRamUsed');
                var dTotal = document.getElementById('dashRamTotal');
                var dPct = document.getElementById('dashRamPercent');
                var dRam = document.getElementById('dashRamFree');
                var dRamMin = document.getElementById('dashRamMin');

                if (data.freeHeap !== undefined) {
                    var freeKB = Math.round(data.freeHeap / 1024);
                    var totKB = (data.totalHeap !== undefined && data.totalHeap > 0) ? Math.round(data.totalHeap / 1024) : 328;
                    var usedKB = Math.max(0, totKB - freeKB);
                    var pct = totKB > 0 ? Math.round((usedKB / totKB) * 100) : 0;

                    if (dUsed) dUsed.innerText = usedKB;
                    if (dTotal) dTotal.innerText = totKB;
                    if (dRam) dRam.innerText = freeKB;
                    if (dPct) {
                        dPct.innerText = pct + '%';
                        if (pct >= 85) dPct.style.color = '#ef4444';
                        else if (pct >= 70) dPct.style.color = '#f59e0b';
                        else dPct.style.color = '#10b981';
                    }
                }
                if (data.minFreeHeap !== undefined && dRamMin) dRamMin.innerText = Math.round(data.minFreeHeap / 1024);

                var dUptime = document.getElementById('dashUptime');
                if (data.uptimeSec !== undefined && dUptime) {
                    var up = data.uptimeSec;
                    var uh = Math.floor(up / 3600);
                    var um = Math.floor((up % 3600) / 60);
                    var us = up % 60;
                    dUptime.innerText = (uh > 0 ? uh + 'h ' : '') + ('0' + um).slice(-2) + 'm ' + ('0' + us).slice(-2) + 's';
                }

                var dLog = document.getElementById('dashLogCount');
                if (data.logCount !== undefined && dLog) dLog.innerText = data.logCount;

                // Cập nhật đồng bộ trạng thái Stress Test
                if (data.stressActive !== undefined) {
                    updateStressUI(data.stressActive, data.stressSec);
                }
            }
        })
        .catch(function(e) {
            fetchingSensor = false;
        });
}

fetchSensorData();
setInterval(fetchSensorData, 1500);

var pollTimer = null;
var pollCount = 0;

function submitForm(e) {
    e.preventDefault();
    var btn = document.getElementById('btnSubmit');
    var alertBox = document.getElementById('alertBox');
    alertBox.style.display = 'none';

    var ssid = document.getElementById('ssid').value.trim();
    if (!ssid) {
        showAlert('error', 'Vui lòng nhập hoặc chọn tên Wi-Fi (SSID)!');
        return;
    }

    btn.disabled = true;
    btn.innerHTML = '<span class="spinner-sm"></span> Đang thử kết nối vào ' + ssid + '...';

    var form = document.getElementById('wifiForm');
    var formData = new FormData(form);

    fetch('/save', {
        method: 'POST',
        body: formData
    })
    .then(r => r.json())
    .then(res => {
        if (!res.success) {
            showAlert('error', res.error || 'Lỗi gửi yêu cầu!');
            btn.disabled = false;
            btn.innerHTML = 'Lưu & Kết Nối Ngay';
            return;
        }
        pollStatus();
    })
    .catch(err => {
        pollStatus();
    });
}

function pollStatus() {
    pollCount = 0;
    if (pollTimer) clearInterval(pollTimer);
    pollTimer = setInterval(function() {
        pollCount++;
        fetch('/status')
        .then(r => r.json())
        .then(data => {
            if (data.state === 'success') {
                clearInterval(pollTimer);
                showSuccessView(data.ip);
            } else if (data.state === 'failed') {
                clearInterval(pollTimer);
                showAlert('error', '<b>KẾT NỐI THẤT BẠI:</b> ' + (data.error || 'Sai mật khẩu hoặc mạng không phản hồi!') + '<br>Vui lòng kiểm tra lại mật khẩu và thử lại.');
                var btn = document.getElementById('btnSubmit');
                btn.disabled = false;
                btn.innerHTML = 'Thử Lại';
                document.getElementById('password').focus();
            } else {
                if (pollCount > 20) {
                    clearInterval(pollTimer);
                    showAlert('error', 'Hết thời gian chờ (Timeout)! Mạng không phản hồi.');
                    var btn = document.getElementById('btnSubmit');
                    btn.disabled = false;
                    btn.innerHTML = 'Thử Lại';
                }
            }
        })
        .catch(err => {
            if (pollCount > 20) {
                clearInterval(pollTimer);
                showAlert('error', 'Mất kết nối với thiết bị. Vui lòng kiểm tra lại kết nối Wi-Fi.');
                var btn = document.getElementById('btnSubmit');
                btn.disabled = false;
                btn.innerHTML = 'Thử Lại';
            }
        });
    }, 1000);
}

function showSuccessView(ip) {
    document.getElementById('setupView').style.display = 'none';
    document.getElementById('successView').style.display = 'block';
    document.getElementById('resSSID').innerText = document.getElementById('ssid').value;
    document.getElementById('resIP').innerText = ip || 'Đang cập nhật...';

    var totalSec = 3;
    var timeLeft = totalSec;
    var countdownEl = document.getElementById('countdownText');
    var progressEl = document.getElementById('progressBar');
    countdownEl.innerText = timeLeft + 's';
    progressEl.style.width = '100%';

    var countTimer = setInterval(function() {
        timeLeft--;
        if (timeLeft > 0) {
            countdownEl.innerText = timeLeft + 's';
            progressEl.style.width = (timeLeft / totalSec * 100) + '%';
        } else {
            clearInterval(countTimer);
            countdownEl.innerText = 'ĐÃ TẮT';
            countdownEl.style.color = '#94a3b8';
            progressEl.style.width = '0%';
            document.getElementById('countdownStatusText').innerText = 'Điểm phát Access Point đã tắt.';
            fetch('/shutdown-ap', { method: 'POST' }).catch(function(){});
        }
    }, 1000);
}

function showAlert(type, msg) {
    var b = document.getElementById('alertBox');
    b.className = 'alert-box alert-' + type;
    b.innerHTML = msg;
    b.style.display = 'block';
}

// ==========================================
// TAB NAVIGATION & LED RGB STUDIO
// ==========================================
function switchTab(tab) {
    var btnWifi = document.getElementById('tabBtnWifi');
    var btnSensor = document.getElementById('tabBtnSensor');
    var btnLed = document.getElementById('tabBtnLed');
    var btnChart = document.getElementById('tabBtnChart');
    var cWifi = document.getElementById('tabContentWifi');
    var cSensor = document.getElementById('tabContentSensor');
    var cLed = document.getElementById('tabContentLed');
    var cChart = document.getElementById('tabContentChart');

    if (btnWifi) btnWifi.classList.remove('active');
    if (btnSensor) btnSensor.classList.remove('active');
    if (btnLed) btnLed.classList.remove('active');
    if (btnChart) btnChart.classList.remove('active');
    if (cWifi) cWifi.style.display = 'none';
    if (cSensor) cSensor.style.display = 'none';
    if (cLed) cLed.style.display = 'none';
    if (cChart) cChart.style.display = 'none';

    if (tab === 'wifi') {
        if (btnWifi) btnWifi.classList.add('active');
        if (cWifi) cWifi.style.display = 'block';
    } else if (tab === 'sensor') {
        if (btnSensor) btnSensor.classList.add('active');
        if (cSensor) cSensor.style.display = 'block';
        fetchSensorSettings();
    } else if (tab === 'led') {
        if (btnLed) btnLed.classList.add('active');
        if (cLed) cLed.style.display = 'block';
        drawColorWheel();
        updateWheelHandle();
    } else if (tab === 'chart') {
        if (btnChart) btnChart.classList.add('active');
        if (cChart) cChart.style.display = 'block';
        syncDeviceTime();
        loadHistoryChart();
    }
}

var currentLedState = {
    mode: 2,
    r: 168,
    g: 85,
    b: 247,
    brightness: 220,
    speed: 50
};

function componentToHex(c) {
    var hex = c.toString(16);
    return hex.length == 1 ? '0' + hex : hex;
}
function rgbToHex(r, g, b) {
    return '#' + componentToHex(r) + componentToHex(g) + componentToHex(b);
}
function hexToRgb(hex) {
    var res = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex);
    return res ? {
        r: parseInt(res[1], 16),
        g: parseInt(res[2], 16),
        b: parseInt(res[3], 16)
    } : null;
}

// Chuyen doi HSL sang RGB
function hue2rgb(p, q, t) {
    if (t < 0) t += 1;
    if (t > 1) t -= 1;
    if (t < 1/6) return p + (q - p) * 6 * t;
    if (t < 1/2) return q;
    if (t < 2/3) return p + (q - p) * (2/3 - t) * 6;
    return p;
}
function hslToRgb(h, s, l) {
    var r, g, b;
    if (s === 0) {
        r = g = b = l;
    } else {
        var q = l < 0.5 ? l * (1 + s) : l + s - l * s;
        var p = 2 * l - q;
        r = hue2rgb(p, q, h + 1/3);
        g = hue2rgb(p, q, h);
        b = hue2rgb(p, q, h - 1/3);
    }
    return [Math.round(r * 255), Math.round(g * 255), Math.round(b * 255)];
}

// Chuyen doi RGB sang HSL de tinh vi tri Handle tren Banh xe mau
function rgbToHsl(r, g, b) {
    r /= 255; g /= 255; b /= 255;
    var max = Math.max(r, g, b), min = Math.min(r, g, b);
    var h = 0, s = 0, l = (max + min) / 2;
    if (max !== min) {
        var d = max - min;
        s = l > 0.5 ? d / (2 - max - min) : d / (max + min);
        switch (max) {
            case r: h = (g - b) / d + (g < b ? 6 : 0); break;
            case g: h = (b - r) / d + 2; break;
            case b: h = (r - g) / d + 4; break;
        }
        h /= 6;
    }
    return { h: h, s: s, l: l };
}

// ==========================================
// CIRCULAR COLOR WHEEL CANVAS LOGIC
// ==========================================
var wheelDrawn = false;
var isDraggingWheel = false;
var wheelRadius = 125; // ban kinh 125px trong canvas 260x260

function drawColorWheel() {
    if (wheelDrawn) return;
    var canvas = document.getElementById('colorWheelCanvas');
    if (!canvas) return;
    var ctx = canvas.getContext('2d');
    var imgData = ctx.createImageData(260, 260);
    var data = imgData.data;

    for (var y = 0; y < 260; y++) {
        for (var x = 0; x < 260; x++) {
            var dx = x - 130;
            var dy = y - 130;
            var dist = Math.sqrt(dx * dx + dy * dy);
            var idx = (y * 260 + x) * 4;

            if (dist <= wheelRadius) {
                var angle = Math.atan2(dy, dx) * 180 / Math.PI;
                var hue = (angle + 360) % 360;
                var sat = dist / wheelRadius;
                var rgb = hslToRgb(hue / 360, sat, 0.5);

                data[idx] = rgb[0];
                data[idx + 1] = rgb[1];
                data[idx + 2] = rgb[2];
                data[idx + 3] = 255;
            } else if (dist <= 130) {
                // Khử răng cưa viền ngoài
                var alpha = Math.max(0, (130 - dist) / 5);
                data[idx + 3] = Math.round(alpha * 255);
            } else {
                data[idx + 3] = 0;
            }
        }
    }
    ctx.putImageData(imgData, 0, 0);
    wheelDrawn = true;
}

var wheelRafId = null;
var pendingWheelEvent = null;

function updateWheelHandle() {
    var hex = rgbToHex(currentLedState.r, currentLedState.g, currentLedState.b).toUpperCase();
    var handle = document.getElementById('wheelHandle');
    var badge = document.getElementById('colorPreviewBadge');
    var hexText = document.getElementById('hexColorVal');
    if (!handle) return;

    var hsl = rgbToHsl(currentLedState.r, currentLedState.g, currentLedState.b);
    var rad = hsl.h * 2 * Math.PI;
    var dist = hsl.s * wheelRadius;

    var hx = 130 + dist * Math.cos(rad);
    var hy = 130 + dist * Math.sin(rad);

    handle.style.transform = 'translate3d(' + (hx - 14) + 'px, ' + (hy - 14) + 'px, 0)';
    handle.style.backgroundColor = hex;
    if (badge) badge.style.backgroundColor = hex;
    if (hexText) hexText.innerText = hex;
}

function processWheelPointer() {
    wheelRafId = null;
    if (!pendingWheelEvent) return;
    var e = pendingWheelEvent;
    pendingWheelEvent = null;

    var canvas = document.getElementById('colorWheelCanvas');
    if (!canvas) return;
    var rect = canvas.getBoundingClientRect();
    var clientX = e.touches ? e.touches[0].clientX : e.clientX;
    var clientY = e.touches ? e.touches[0].clientY : e.clientY;

    var x = clientX - rect.left;
    var y = clientY - rect.top;

    var dx = x - 130;
    var dy = y - 130;
    var dist = Math.sqrt(dx * dx + dy * dy);

    if (dist > wheelRadius) {
        dx = dx * (wheelRadius / dist);
        dy = dy * (wheelRadius / dist);
        dist = wheelRadius;
    }

    var angle = Math.atan2(dy, dx) * 180 / Math.PI;
    var hue = (angle + 360) % 360;
    var sat = dist / wheelRadius;
    var rgb = hslToRgb(hue / 360, sat, 0.5);

    currentLedState.r = rgb[0];
    currentLedState.g = rgb[1];
    currentLedState.b = rgb[2];

    var hex = rgbToHex(rgb[0], rgb[1], rgb[2]).toUpperCase();

    // Cap nhat vi tri Handle bang GPU Compositor translate3d
    var handle = document.getElementById('wheelHandle');
    if (handle) {
        handle.style.transform = 'translate3d(' + (130 + dx - 14) + 'px, ' + (130 + dy - 14) + 'px, 0)';
        handle.style.backgroundColor = hex;
    }
    var badge = document.getElementById('colorPreviewBadge');
    if (badge) badge.style.backgroundColor = hex;
    var hexText = document.getElementById('hexColorVal');
    if (hexText) hexText.innerText = hex;

    // Neu dang chon mau ma mode dang la Tat, Rainbow hoac Bar Club -> tu chuyen sang Breathing
    if (currentLedState.mode !== 1 && currentLedState.mode !== 2) {
        currentLedState.mode = 2; // Breathing
        updateLedUI();
    }

    sendLedUpdate(false);
}

function handleWheelPointerEvent(e) {
    pendingWheelEvent = e;
    if (!wheelRafId) {
        wheelRafId = requestAnimationFrame(processWheelPointer);
    }
}

function isInsideWheel(e) {
    var canvas = document.getElementById('colorWheelCanvas');
    if (!canvas) return false;
    var rect = canvas.getBoundingClientRect();
    var clientX = e.touches ? e.touches[0].clientX : e.clientX;
    var clientY = e.touches ? e.touches[0].clientY : e.clientY;
    var dx = clientX - (rect.left + 130);
    var dy = clientY - (rect.top + 130);
    return (dx * dx + dy * dy) <= (wheelRadius + 4) * (wheelRadius + 4);
}

function initColorWheelEvents() {
    var container = document.getElementById('colorWheelContainer');
    if (!container) return;

    var onDown = function(e) {
        if (!isInsideWheel(e)) {
            isDraggingWheel = false;
            return;
        }
        isDraggingWheel = true;
        handleWheelPointerEvent(e);
        if (e.cancelable) e.preventDefault();
    };
    var onMove = function(e) {
        if (!isDraggingWheel) return;
        handleWheelPointerEvent(e);
        if (e.cancelable) e.preventDefault();
    };
    var onUp = function(e) {
        if (isDraggingWheel) {
            isDraggingWheel = false;
            if (pendingWheelEvent) {
                processWheelPointer();
            }
        }
    };

    container.addEventListener('mousedown', onDown);
    window.addEventListener('mousemove', onMove);
    window.addEventListener('mouseup', onUp);

    container.addEventListener('touchstart', onDown, { passive: false });
    window.addEventListener('touchmove', onMove, { passive: false });
    window.addEventListener('touchend', onUp);
    window.addEventListener('touchcancel', onUp);
}

function updateLedUI() {
    for (var i = 0; i <= 5; i++) {
        var c = document.getElementById('cardMode' + i);
        if (c) {
            if (currentLedState.mode === i) c.classList.add('active');
            else c.classList.remove('active');
        }
    }

    var colorSec = document.getElementById('ledColorSection');
    var speedSec = document.getElementById('ledSpeedSection');

    // Mau sac ap dung cho che do Static (1) va Breathing (2)
    if (currentLedState.mode === 1 || currentLedState.mode === 2) {
        colorSec.style.display = 'block';
        drawColorWheel();
        updateWheelHandle();
    } else {
        colorSec.style.display = 'none';
    }

    // Toc do ap dung cho Breathing (2), Rainbow (3), Bar Club (4)
    if (currentLedState.mode === 2 || currentLedState.mode === 3 || currentLedState.mode === 4) {
        speedSec.style.display = 'block';
    } else {
        speedSec.style.display = 'none';
    }

    document.getElementById('ledBrightness').value = currentLedState.brightness;
    document.getElementById('brightVal').innerText = Math.round(currentLedState.brightness / 255 * 100) + '%';

    document.getElementById('ledSpeed').value = currentLedState.speed;
    document.getElementById('speedVal').innerText = currentLedState.speed;
}

function selectLedMode(m) {
    currentLedState.mode = m;
    updateLedUI();
    sendLedUpdate(false);
}

function pickPresetColor(hex) {
    var rgb = hexToRgb(hex);
    if (rgb) {
        currentLedState.r = rgb.r;
        currentLedState.g = rgb.g;
        currentLedState.b = rgb.b;
        if (currentLedState.mode !== 1 && currentLedState.mode !== 2) {
            currentLedState.mode = 2; // Chuyen sang Breathing
        }
        updateLedUI();
        sendLedUpdate(false);
    }
}

function onBrightnessChange(e) {
    currentLedState.brightness = parseInt(e.target.value);
    document.getElementById('brightVal').innerText = Math.round(currentLedState.brightness / 255 * 100) + '%';
    sendLedUpdate(false);
}

function onSpeedChange(e) {
    currentLedState.speed = parseInt(e.target.value);
    document.getElementById('speedVal').innerText = currentLedState.speed;
    sendLedUpdate(false);
}

// Pipelining non-blocking real-time send logic
var inFlight = false;
var hasPending = false;
var nextAllowedTime = 0;

function sendLedUpdate(isSave) {
    if (isSave) {
        var params = new URLSearchParams({
            mode: currentLedState.mode,
            r: currentLedState.r,
            g: currentLedState.g,
            b: currentLedState.b,
            brightness: currentLedState.brightness,
            speed: currentLedState.speed,
            save: '1'
        });
        fetch('/api/led?' + params.toString(), { method: 'POST' })
            .then(function() {
                var t = document.getElementById('ledToast');
                t.style.display = 'block';
                setTimeout(function() { t.style.display = 'none'; }, 2500);
            })
            .catch(function(){});
        return;
    }

    hasPending = true;
    scheduleNextSend();
}

function scheduleNextSend() {
    if (inFlight) return;
    if (!hasPending) return;

    var now = Date.now();
    var waitMs = Math.max(0, nextAllowedTime - now);

    setTimeout(function() {
        if (inFlight || !hasPending) return;

        inFlight = true;
        hasPending = false;
        nextAllowedTime = Date.now() + 20; // 50 FPS cao cap, cuon vuot cuc muot

        var params = new URLSearchParams({
            mode: currentLedState.mode,
            r: currentLedState.r,
            g: currentLedState.g,
            b: currentLedState.b,
            brightness: currentLedState.brightness,
            speed: currentLedState.speed,
            save: '0'
        });

        fetch('/api/led?' + params.toString(), { method: 'POST', keepalive: true })
            .then(function() {
                inFlight = false;
                if (hasPending) {
                    scheduleNextSend();
                }
            })
            .catch(function() {
                inFlight = false;
                if (hasPending) {
                    scheduleNextSend();
                }
            });
    }, waitMs);
}

function saveLedConfig() {
    sendLedUpdate(true);
}

function resetLedToDefault() {
    fetch('/api/led/reset', { method: 'POST' })
        .then(r => r.json())
        .then(data => {
            currentLedState.mode = 2;
            currentLedState.r = 168;
            currentLedState.g = 85;
            currentLedState.b = 247;
            currentLedState.brightness = 220;
            currentLedState.speed = 50;
            updateLedUI();
            var toast = document.getElementById('ledToast');
            toast.innerText = 'Đã khôi phục LED về Tím Breathing mặc định!';
            toast.style.display = 'block';
            setTimeout(function() { toast.style.display = 'none'; }, 2500);
        })
        .catch(function(){});
}

function fetchLedConfig() {
    fetch('/api/led')
        .then(r => r.json())
        .then(data => {
            if (data) {
                currentLedState.mode = data.mode;
                currentLedState.r = data.r;
                currentLedState.g = data.g;
                currentLedState.b = data.b;
                currentLedState.brightness = data.brightness;
                currentLedState.speed = data.speed;
                updateLedUI();
            }
        })
        .catch(function(){});
}

// ==========================================
// SENSOR SETTINGS LOGIC
// ==========================================
function fetchSensorSettings() {
    fetch('/api/sensor-cfg')
        .then(r => r.json())
        .then(data => {
            if (data) {
                if (data.tempAlert !== undefined) document.getElementById('cfgTempAlert').value = data.tempAlert;
                if (data.humAlert !== undefined) document.getElementById('cfgHumAlert').value = data.humAlert;
                if (data.readInterval !== undefined) document.getElementById('cfgReadInterval').value = data.readInterval;
                if (data.sendInterval !== undefined) document.getElementById('cfgSendInterval').value = data.sendInterval;
            }
        })
        .catch(function(){});
}

function saveSensorSettings() {
    var t = parseFloat(document.getElementById('cfgTempAlert').value);
    var h = parseFloat(document.getElementById('cfgHumAlert').value);
    var r = parseInt(document.getElementById('cfgReadInterval').value);
    var s = parseInt(document.getElementById('cfgSendInterval').value);

    if (isNaN(t) || t < 20 || t > 80) {
        alert('Ngưỡng nhiệt độ phải từ 20°C đến 80°C!');
        return;
    }
    if (isNaN(h) || h < 30 || h > 99) {
        alert('Ngưỡng độ ẩm phải từ 30% đến 99%!');
        return;
    }
    if (isNaN(r) || r < 1 || r > 60) {
        alert('Chu kỳ đọc phải từ 1 đến 60 giây!');
        return;
    }
    if (isNaN(s) || s < 2 || s > 300) {
        alert('Chu kỳ gửi phải từ 2 đến 300 giây!');
        return;
    }

    var params = new URLSearchParams({
        temp_alert: t,
        hum_alert: h,
        read_interval: r,
        send_interval: s
    });

    fetch('/api/sensor-cfg?' + params.toString(), { method: 'POST' })
        .then(r => r.json())
        .then(data => {
            var toast = document.getElementById('sensorToast');
            toast.innerText = 'Đã lưu cấu hình cảm biến thành công!';
            toast.style.display = 'block';
            setTimeout(function() { toast.style.display = 'none'; }, 2500);
        })
        .catch(function(){});
}

function resetSensorSettings() {
    fetch('/api/sensor-cfg?reset=1', { method: 'POST' })
        .then(r => r.json())
        .then(data => {
            fetchSensorSettings();
            var toast = document.getElementById('sensorToast');
            toast.innerText = 'Đã khôi phục thông số cảm biến về mặc định!';
            toast.style.display = 'block';
            setTimeout(function() { toast.style.display = 'none'; }, 2500);
        })
        .catch(function(){});
}

// ==========================================
// STRESS TEST 100% CPU DUAL-CORE
// ==========================================
var isStressRunning = false;
var stressLocalTimer = null;
var stressLocalSec = 0;

function formatStressTimer(sec) {
    sec = Math.floor(sec || 0);
    var m = Math.floor(sec / 60);
    var s = sec % 60;
    return (m < 10 ? '0' : '') + m + ':' + (s < 10 ? '0' : '') + s;
}

function updateStressUI(running, sec) {
    isStressRunning = !!running;
    var btn = document.getElementById('btnStressToggle');
    var timerBox = document.getElementById('stressTimerBox');
    var timerVal = document.getElementById('stressTimerVal');

    if (isStressRunning) {
        if (sec !== undefined) stressLocalSec = sec;
        if (timerBox) timerBox.style.display = 'flex';
        if (timerVal) timerVal.innerText = formatStressTimer(stressLocalSec);
        if (btn) {
            btn.className = 'btn-stress-stop';
            btn.innerText = 'Dừng Ép Tải CPU';
        }
        if (!stressLocalTimer) {
            stressLocalTimer = setInterval(function() {
                stressLocalSec++;
                if (timerVal) timerVal.innerText = formatStressTimer(stressLocalSec);
            }, 1000);
        }
    } else {
        if (stressLocalTimer) {
            clearInterval(stressLocalTimer);
            stressLocalTimer = null;
        }
        stressLocalSec = 0;
        if (timerBox) timerBox.style.display = 'none';
        if (btn) {
            btn.className = 'btn-stress-start';
            btn.innerText = 'Bắt Đầu Ép Tải 100% CPU';
        }
    }
}

function toggleStressTest() {
    var nextAction = isStressRunning ? 'stop' : 'start';
    var btn = document.getElementById('btnStressToggle');
    if (btn) {
        btn.disabled = true;
        btn.innerText = (nextAction === 'start') ? 'Đang kích hoạt...' : 'Đang dừng...';
    }

    fetch('/api/stress-test?action=' + nextAction, { method: 'POST' })
        .then(function(r) { return r.json(); })
        .then(function(data) {
            if (btn) btn.disabled = false;
            if (data) {
                var running = (data.running !== undefined) ? data.running : (data.stressActive !== undefined ? data.stressActive : false);
                var sec = (data.elapsedSec !== undefined) ? data.elapsedSec : (data.stressSec !== undefined ? data.stressSec : 0);
                if (data.chipTemp !== undefined) {
                    var cEl = document.getElementById('liveChipTemp');
                    if (cEl) cEl.innerText = data.chipTemp.toFixed(1);
                    var sbEl = document.getElementById('stressChipBadge');
                    if (sbEl) sbEl.innerText = data.chipTemp.toFixed(1) + ' °C';
                }
                if (data.cpu !== undefined) {
                    var cpuEl = document.getElementById('liveCpuLoad');
                    if (cpuEl) cpuEl.innerText = data.cpu.toFixed(1);
                    var scbEl = document.getElementById('stressCpuBadge');
                    if (scbEl) {
                        scbEl.innerText = 'CPU: ' + Math.round(data.cpu) + '%';
                        if (running || data.cpu >= 90) scbEl.classList.add('active');
                        else scbEl.classList.remove('active');
                    }
                }
                if (data.cpu0 !== undefined) {
                    var c0El = document.getElementById('stressCpu0Val');
                    if (c0El) c0El.innerText = Math.round(data.cpu0) + '%';
                }
                if (data.cpu1 !== undefined) {
                    var c1El = document.getElementById('stressCpu1Val');
                    if (c1El) c1El.innerText = Math.round(data.cpu1) + '%';
                }
                updateStressUI(running, sec);
            }
        })
        .catch(function(e) {
            if (btn) btn.disabled = false;
        });
}

// ==========================================
// TAB 4: BIỂU ĐỒ LỊCH SỬ ĐO SHT31
// ==========================================
var currentChartHours = 24;
var cachedChartPoints = [];
var chartRenderedCoords = [];

function setChartFilter(hours) {
    currentChartHours = hours;
    var b24 = document.getElementById('filterBtn24');
    var b6 = document.getElementById('filterBtn6');
    var b1 = document.getElementById('filterBtn1');
    if (b24) b24.className = (hours === 24) ? 'btn-filter active' : 'btn-filter';
    if (b6) b6.className = (hours === 6) ? 'btn-filter active' : 'btn-filter';
    if (b1) b1.className = (hours === 1) ? 'btn-filter active' : 'btn-filter';
    loadHistoryChart();
}

function downloadCsv() {
    window.location.href = '/api/history/csv';
}

function syncDeviceTime() {
    var epoch = Math.floor(Date.now() / 1000);
    fetch('/api/sync-time?epoch=' + epoch, { method: 'POST' }).catch(function(){});
}

function formatChartTime(ts, isEpoch, detail) {
    var d;
    if (isEpoch || ts > 1000000000) {
        d = new Date(ts * 1000);
    } else {
        var lastPt = (cachedChartPoints && cachedChartPoints.length > 0) ? cachedChartPoints[cachedChartPoints.length - 1] : null;
        var offsetSec = lastPt ? Math.max(0, lastPt[0] - ts) : 0;
        var clientNowSec = Math.floor(Date.now() / 1000);
        d = new Date((clientNowSec - offsetSec) * 1000);
    }
    var hh = ('0' + d.getHours()).slice(-2);
    var mm = ('0' + d.getMinutes()).slice(-2);
    if (detail) {
        var ss = ('0' + d.getSeconds()).slice(-2);
        return hh + ':' + mm + ':' + ss;
    }
    return hh + ':' + mm;
}

function loadHistoryChart() {
    var countEl = document.getElementById('chartDataCount');
    if (countEl) countEl.innerText = 'Đang tải...';

    fetch('/api/history?hours=' + currentChartHours)
        .then(function(res) { return res.json(); })
        .then(function(data) {
            cachedChartPoints = (data && (data.points || data.data)) ? (data.points || data.data) : [];
            
            var countText = cachedChartPoints.length + ' mẫu';
            if (cachedChartPoints.length > 0) {
                var minT = cachedChartPoints[0][0];
                var maxT = cachedChartPoints[cachedChartPoints.length - 1][0];
                var spanSec = Math.max(0, maxT - minT);
                var spanMin = Math.round(spanSec / 60);
                if (spanMin >= 60) {
                    var spanH = Math.floor(spanMin / 60);
                    var remM = spanMin % 60;
                    countText = 'Đã lưu: ' + spanH + 'h' + (remM > 0 ? remM + 'm' : '') + ' (' + cachedChartPoints.length + ' mẫu)';
                } else {
                    countText = 'Đã lưu: ' + Math.max(1, spanMin) + ' phút (' + cachedChartPoints.length + ' mẫu)';
                }
            }
            if (countEl) countEl.innerText = countText;

            if (cachedChartPoints.length === 0) {
                var tMin = document.getElementById('statTempMin');
                var tMax = document.getElementById('statTempMax');
                var hMin = document.getElementById('statHumMin');
                var hMax = document.getElementById('statHumMax');
                if (tMin) tMin.innerText = '--.-';
                if (tMax) tMax.innerText = '--.-';
                if (hMin) hMin.innerText = '--.-';
                if (hMax) hMax.innerText = '--.-';
                renderEmptyChart('Chưa có điểm dữ liệu trong ' + currentChartHours + 'h qua');
                return;
            }

            var minT = 999.0, maxT = -999.0;
            var minH = 999.0, maxH = -999.0;
            for (var i = 0; i < cachedChartPoints.length; i++) {
                var p = cachedChartPoints[i];
                var t = p[1];
                var h = p[2];
                if (t < minT) minT = t;
                if (t > maxT) maxT = t;
                if (h < minH) minH = h;
                if (h > maxH) maxH = h;
            }

            var elTmin = document.getElementById('statTempMin');
            var elTmax = document.getElementById('statTempMax');
            var elHmin = document.getElementById('statHumMin');
            var elHmax = document.getElementById('statHumMax');
            if (elTmin) elTmin.innerText = minT.toFixed(1);
            if (elTmax) elTmax.innerText = maxT.toFixed(1);
            if (elHmin) elHmin.innerText = minH.toFixed(1);
            if (elHmax) elHmax.innerText = maxH.toFixed(1);

            renderHistoryCanvas(cachedChartPoints, minT, maxT, minH, maxH, -1);
        })
        .catch(function(err) {
            if (countEl) countEl.innerText = 'Lỗi nạp';
            renderEmptyChart('Không thể kết nối máy chủ');
        });
}

function renderEmptyChart(msg) {
    var canvas = document.getElementById('historyCanvas');
    if (!canvas) return;
    var dpr = window.devicePixelRatio || 1;
    var rect = canvas.getBoundingClientRect();
    var w = rect.width || 340;
    var h = 210;
    canvas.width = w * dpr;
    canvas.height = h * dpr;
    var ctx = canvas.getContext('2d');
    ctx.scale(dpr, dpr);
    ctx.clearRect(0, 0, w, h);
    ctx.fillStyle = '#64748b';
    ctx.font = '12px system-ui, sans-serif';
    ctx.textAlign = 'center';
    ctx.textBaseline = 'middle';
    ctx.fillText(msg, w / 2, h / 2);
}

function renderHistoryCanvas(points, minT, maxT, minH, maxH, highlightIdx) {
    var canvas = document.getElementById('historyCanvas');
    if (!canvas) return;
    var dpr = window.devicePixelRatio || 1;
    var rect = canvas.getBoundingClientRect();
    var w = rect.width || 340;
    var h = 210;
    canvas.width = w * dpr;
    canvas.height = h * dpr;
    var ctx = canvas.getContext('2d');
    ctx.scale(dpr, dpr);
    ctx.clearRect(0, 0, w, h);

    var padL = 34, padR = 34, padT = 18, padB = 26;
    var plotW = w - padL - padR;
    var plotH = h - padT - padB;

    if (plotW <= 10 || plotH <= 10) return;

    var isEpoch = (points.length > 0 && points[0][0] > 1000000000);

    var scaleMinT = Math.floor(minT - 0.5);
    var scaleMaxT = Math.ceil(maxT + 0.5);
    if (scaleMaxT - scaleMinT < 3) scaleMaxT = scaleMinT + 3;

    var scaleMinH = Math.max(0, Math.floor(minH - 3));
    var scaleMaxH = Math.min(100, Math.ceil(maxH + 3));
    if (scaleMaxH - scaleMinH < 8) scaleMaxH = scaleMinH + 8;

    // Ve luoi ngang (Grid lines)
    var gridSteps = 4;
    ctx.strokeStyle = 'rgba(255, 255, 255, 0.06)';
    ctx.lineWidth = 1;
    ctx.font = '10px system-ui, sans-serif';

    for (var g = 0; g <= gridSteps; g++) {
        var gy = padT + (plotH / gridSteps) * g;
        ctx.beginPath();
        ctx.moveTo(padL, gy);
        ctx.lineTo(padL + plotW, gy);
        ctx.stroke();

        var tVal = scaleMaxT - (g / gridSteps) * (scaleMaxT - scaleMinT);
        ctx.fillStyle = '#f43f5e';
        ctx.textAlign = 'right';
        ctx.textBaseline = 'middle';
        ctx.fillText(tVal.toFixed(1) + '°', padL - 5, gy);

        var hVal = scaleMaxH - (g / gridSteps) * (scaleMaxH - scaleMinH);
        ctx.fillStyle = '#38bdf8';
        ctx.textAlign = 'left';
        ctx.textBaseline = 'middle';
        ctx.fillText(Math.round(hVal) + '%', padL + plotW + 5, gy);
    }

    // Vung nen canh bao nhiet do: do nhat khi > 36°C, xanh nhat khi < 31°C
    var TEMP_HIGH = 36.0;
    var TEMP_LOW  = 31.0;
    var tRange = scaleMaxT - scaleMinT;
    ctx.save();
    ctx.beginPath();
    ctx.rect(padL, padT, plotW, plotH);
    ctx.clip();
    // Vung nong (> TEMP_HIGH): to do nhat tu nguong len den dinh
    if (scaleMaxT > TEMP_HIGH) {
        var yHigh = padT + plotH - ((TEMP_HIGH - scaleMinT) / tRange) * plotH;
        yHigh = Math.max(padT, yHigh);
        ctx.fillStyle = 'rgba(239, 68, 68, 0.10)';
        ctx.fillRect(padL, padT, plotW, yHigh - padT);
    }
    // Vung lanh (< TEMP_LOW): to xanh duong nhat tu nguong xuong day
    if (scaleMinT < TEMP_LOW) {
        var yLow = padT + plotH - ((TEMP_LOW - scaleMinT) / tRange) * plotH;
        yLow = Math.min(padT + plotH, yLow);
        ctx.fillStyle = 'rgba(56, 189, 248, 0.09)';
        ctx.fillRect(padL, yLow, plotW, (padT + plotH) - yLow);
    }
    ctx.restore();

    if (points.length === 1) {
        var singleX = padL + plotW / 2;
        var sYt = padT + plotH - ((points[0][1] - scaleMinT) / (scaleMaxT - scaleMinT)) * plotH;
        var sYh = padT + plotH - ((points[0][2] - scaleMinH) / (scaleMaxH - scaleMinH)) * plotH;
        ctx.fillStyle = '#f43f5e';
        ctx.beginPath(); ctx.arc(singleX, sYt, 5, 0, Math.PI * 2); ctx.fill();
        ctx.fillStyle = '#38bdf8';
        ctx.beginPath(); ctx.arc(singleX, sYh, 5, 0, Math.PI * 2); ctx.fill();
        chartRenderedCoords = [{ x: singleX, yT: sYt, yH: sYh, pt: points[0] }];
        return;
    }

    var minTime = points[0][0];
    var maxTime = points[points.length - 1][0];
    var timeSpan = maxTime - minTime;
    if (timeSpan <= 0) timeSpan = points.length;

    chartRenderedCoords = [];
    for (var i = 0; i < points.length; i++) {
        var pt = points[i];
        var x = (timeSpan > 0 && maxTime !== minTime)
            ? padL + ((pt[0] - minTime) / timeSpan) * plotW
            : padL + (i / (points.length - 1)) * plotW;
        var yT = padT + plotH - ((pt[1] - scaleMinT) / (scaleMaxT - scaleMinT)) * plotH;
        var yH = padT + plotH - ((pt[2] - scaleMinH) / (scaleMaxH - scaleMinH)) * plotH;
        chartRenderedCoords.push({ x: x, yT: yT, yH: yH, pt: pt });
    }

    // 1. Ve vung phu gradient Do Am (Xanh Cyan)
    var gradH = ctx.createLinearGradient(0, padT, 0, padT + plotH);
    gradH.addColorStop(0, 'rgba(56, 189, 248, 0.22)');
    gradH.addColorStop(1, 'rgba(56, 189, 248, 0.00)');
    ctx.beginPath();
    ctx.moveTo(chartRenderedCoords[0].x, padT + plotH);
    for (var i = 0; i < chartRenderedCoords.length; i++) {
        ctx.lineTo(chartRenderedCoords[i].x, chartRenderedCoords[i].yH);
    }
    ctx.lineTo(chartRenderedCoords[chartRenderedCoords.length - 1].x, padT + plotH);
    ctx.closePath();
    ctx.fillStyle = gradH;
    ctx.fill();

    // 2. Ve duong Do Am
    ctx.beginPath();
    ctx.strokeStyle = '#38bdf8';
    ctx.lineWidth = 2;
    ctx.lineJoin = 'round';
    for (var i = 0; i < chartRenderedCoords.length; i++) {
        if (i === 0) ctx.moveTo(chartRenderedCoords[i].x, chartRenderedCoords[i].yH);
        else ctx.lineTo(chartRenderedCoords[i].x, chartRenderedCoords[i].yH);
    }
    ctx.stroke();

    // 3. Ve vung phu gradient Nhiet Do (Do Hong)
    var gradT = ctx.createLinearGradient(0, padT, 0, padT + plotH);
    gradT.addColorStop(0, 'rgba(244, 63, 94, 0.25)');
    gradT.addColorStop(1, 'rgba(244, 63, 94, 0.00)');
    ctx.beginPath();
    ctx.moveTo(chartRenderedCoords[0].x, padT + plotH);
    for (var i = 0; i < chartRenderedCoords.length; i++) {
        ctx.lineTo(chartRenderedCoords[i].x, chartRenderedCoords[i].yT);
    }
    ctx.lineTo(chartRenderedCoords[chartRenderedCoords.length - 1].x, padT + plotH);
    ctx.closePath();
    ctx.fillStyle = gradT;
    ctx.fill();

    // 4. Ve duong Nhiet Do
    ctx.beginPath();
    ctx.strokeStyle = '#f43f5e';
    ctx.lineWidth = 2;
    ctx.lineJoin = 'round';
    for (var i = 0; i < chartRenderedCoords.length; i++) {
        if (i === 0) ctx.moveTo(chartRenderedCoords[i].x, chartRenderedCoords[i].yT);
        else ctx.lineTo(chartRenderedCoords[i].x, chartRenderedCoords[i].yT);
    }
    ctx.stroke();

    // 5. Nhan thoi gian truc X duoi cung
    ctx.fillStyle = '#64748b';
    ctx.font = '10px system-ui, sans-serif';
    ctx.textAlign = 'left';
    ctx.fillText(formatChartTime(minTime, isEpoch), padL, padT + plotH + 16);
    ctx.textAlign = 'right';
    ctx.fillText(formatChartTime(maxTime, isEpoch), padL + plotW, padT + plotH + 16);
    if (points.length > 2) {
        var midIdx = Math.floor(points.length / 2);
        ctx.textAlign = 'center';
        ctx.fillText(formatChartTime(points[midIdx][0], isEpoch), chartRenderedCoords[midIdx].x, padT + plotH + 16);
    }

    // 6. Highlight diem dang duoc hover/touch
    if (highlightIdx >= 0 && highlightIdx < chartRenderedCoords.length) {
        var hp = chartRenderedCoords[highlightIdx];

        // Duong thang dung crosshair
        ctx.beginPath();
        ctx.setLineDash([3, 3]);
        ctx.strokeStyle = 'rgba(255, 255, 255, 0.4)';
        ctx.lineWidth = 1;
        ctx.moveTo(hp.x, padT);
        ctx.lineTo(hp.x, padT + plotH);
        ctx.stroke();
        ctx.setLineDash([]);

        // Cham Nhiet do
        ctx.fillStyle = '#f43f5e';
        ctx.strokeStyle = '#fff';
        ctx.lineWidth = 2;
        ctx.beginPath();
        ctx.arc(hp.x, hp.yT, 5, 0, Math.PI * 2);
        ctx.fill();
        ctx.stroke();

        // Cham Do am
        ctx.fillStyle = '#38bdf8';
        ctx.beginPath();
        ctx.arc(hp.x, hp.yH, 5, 0, Math.PI * 2);
        ctx.fill();
        ctx.stroke();
    }
}

function handleChartHover(clientX, clientY) {
    if (!chartRenderedCoords || chartRenderedCoords.length === 0) return;
    var canvas = document.getElementById('historyCanvas');
    var tooltip = document.getElementById('chartTooltip');
    if (!canvas || !tooltip) return;

    var rect = canvas.getBoundingClientRect();
    var x = clientX - rect.left;
    var y = clientY - rect.top;

    var closestIdx = 0;
    var minDist = 99999;
    for (var i = 0; i < chartRenderedCoords.length; i++) {
        var dist = Math.abs(chartRenderedCoords[i].x - x);
        if (dist < minDist) {
            minDist = dist;
            closestIdx = i;
        }
    }

    var match = chartRenderedCoords[closestIdx];
    var isEpoch = (match.pt[0] > 1000000000);
    var timeStr = formatChartTime(match.pt[0], isEpoch, true);

    tooltip.innerHTML = '<div style="font-weight:700; color:#cbd5e1; margin-bottom:3px;">⏱ ' + timeStr + '</div>' +
        '<div style="color:#f43f5e; font-weight:700;">🌡 Nhiệt độ: ' + match.pt[1].toFixed(1) + ' °C</div>' +
        '<div style="color:#38bdf8; font-weight:700;">💧 Độ ẩm: ' + match.pt[2].toFixed(1) + ' %</div>';

    // Hien thi truoc de tinh kich thuoc
    tooltip.style.display = 'block';
    tooltip.style.visibility = 'hidden';
    
    var tw = tooltip.offsetWidth;
    var th = tooltip.offsetHeight;
    var canvasW = rect.width;
    var canvasH = rect.height;
    
    // Lay vi tri dot thap nhat (dot nao cao hon tren man hinh)
    var dotTopY = Math.min(match.yT, match.yH);
    
    // Mac dinh: dat tooltip phia tren dot, canh giua
    var tipLeft = match.x - tw / 2;
    var tipTop  = dotTopY - th - 12;
    
    // Neu tooltip vuot ra ngoai tren canvas, lat xuong duoi
    if (tipTop < 0) {
        var dotBottomY = Math.max(match.yT, match.yH);
        tipTop = dotBottomY + 12;
    }
    
    // Clamp ngang de khong bi cat
    if (tipLeft < 4) tipLeft = 4;
    if (tipLeft + tw > canvasW - 4) tipLeft = canvasW - tw - 4;
    
    // Clamp doc
    if (tipTop + th > canvasH - 4) tipTop = canvasH - th - 4;
    if (tipTop < 4) tipTop = 4;
    
    tooltip.style.left = tipLeft + 'px';
    tooltip.style.top  = tipTop + 'px';
    tooltip.style.visibility = 'visible';

    renderHistoryCanvas(cachedChartPoints,
        parseFloat(document.getElementById('statTempMin').innerText) || 20,
        parseFloat(document.getElementById('statTempMax').innerText) || 40,
        parseFloat(document.getElementById('statHumMin').innerText) || 30,
        parseFloat(document.getElementById('statHumMax').innerText) || 80,
        closestIdx
    );
}

function hideChartTooltip() {
    var tooltip = document.getElementById('chartTooltip');
    if (tooltip) tooltip.style.display = 'none';
    if (cachedChartPoints && cachedChartPoints.length > 0) {
        renderHistoryCanvas(cachedChartPoints,
            parseFloat(document.getElementById('statTempMin').innerText) || 20,
            parseFloat(document.getElementById('statTempMax').innerText) || 40,
            parseFloat(document.getElementById('statHumMin').innerText) || 30,
            parseFloat(document.getElementById('statHumMax').innerText) || 80,
            -1
        );
    }
}

function initChartEvents() {
    var canvas = document.getElementById('historyCanvas');
    if (!canvas) return;

    canvas.addEventListener('mousemove', function(e) {
        handleChartHover(e.clientX, e.clientY);
    });
    canvas.addEventListener('mouseleave', function() {
        hideChartTooltip();
    });

    canvas.addEventListener('touchstart', function(e) {
        if (e.touches.length > 0) {
            handleChartHover(e.touches[0].clientX, e.touches[0].clientY);
        }
    }, { passive: true });

    canvas.addEventListener('touchmove', function(e) {
        if (e.touches.length > 0) {
            handleChartHover(e.touches[0].clientX, e.touches[0].clientY);
        }
    }, { passive: true });

    canvas.addEventListener('touchend', function() {
        setTimeout(hideChartTooltip, 2500);
    });
}

// Khoi chay khi load trang
document.addEventListener('DOMContentLoaded', function() {
    initColorWheelEvents();
    initChartEvents();
    syncDeviceTime();
    fetchLedConfig();
    fetchSensorSettings();
});
// Fallback chay ngay neu da load
initColorWheelEvents();
initChartEvents();
syncDeviceTime();
fetchLedConfig();
fetchSensorSettings();

function resetWifiCredentials() {
    if (!confirm('Xác nhận: Xóa cấu hình Wi-Fi và khởi động lại?\n\nESP32 sẽ phát lại Hotspot để cài đặt lại từ đầu.')) return;
    var btn = document.getElementById('btnResetWifi');
    if (btn) { btn.disabled = true; btn.textContent = 'Đang xử lý...'; }
    fetch('/api/reset-wifi', { method: 'POST' })
        .then(function() {
            if (btn) { btn.textContent = 'Đang khởi động lại...'; }
        })
        .catch(function() {
            // ESP.restart() lam mat ket noi ngay => fetch se bi loi, day la binh thuong
            if (btn) { btn.textContent = 'Dang khoi dong lai...'; }
        });
}
)rawliteral";
