export function updateGauge(type, value) {
    const gaugeElement = document.querySelector(`.${type}-gauge`);
    if (!gaugeElement) return;

    let percentage = 0;

    switch (type) {
        case 'temp':
            // Range: 0°C to 50°C
            percentage = (value / 50) * 100;
            break;
        case 'ph':
            // Range: 0 to 14
            percentage = (value / 14) * 100;
            break;
        case 'tds':
            // Range: 0 to 1000 ppm
            percentage = (value / 1000) * 100;
            break;
    }

    // Constraints to keep fill within 0-100%
    percentage = Math.min(Math.max(percentage, 0), 100);

    // Calculate rotation degree. 
    // 45deg = 0% fill (hidden entirely below)
    // 225deg = 100% fill (fully covers the top half)
    const rotation = 45 + (percentage / 100) * 180;

    const styleId = `gauge-style-${type}`;
    let styleEl = document.getElementById(styleId);

    if (!styleEl) {
        styleEl = document.createElement('style');
        styleEl.id = styleId;
        document.head.appendChild(styleEl);
    }

    // Maps the calculated rotation to the ::after pseudo-element
    styleEl.innerHTML = `
        .${type}-gauge::after {
            transform: rotate(${rotation}deg) !important;
        }
    `;
}