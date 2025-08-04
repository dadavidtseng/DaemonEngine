//----------------------------------------------------------------------------------------------------
// custom.js
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
document.addEventListener('DOMContentLoaded', function() {

    // 為程式碼區塊添加複製按鈕
    addCopyButtons();

    // 為 API 區塊添加摺疊功能
    addCollapsibleSections();

    // 添加回到頂部按鈕
    addBackToTop();

    // 為外部連結添加圖示
    addExternalLinkIcons();
});

//----------------------------------------------------------------------------------------------------
// 為程式碼區塊添加複製按鈕
function addCopyButtons() {
    const codeBlocks = document.querySelectorAll('.highlight pre');

    codeBlocks.forEach(function(codeBlock) {
        const button = document.createElement('button');
        button.className = 'copy-button';
        button.textContent = 'Copy';
        button.style.cssText = `
            position: absolute;
            top: 8px;
            right: 8px;
            padding: 4px 8px;
            background: #34495e;
            color: white;
            border: none;
            border-radius: 4px;
            font-size: 12px;
            cursor: pointer;
            opacity: 0;
            transition: opacity 0.3s;
        `;

        const container = codeBlock.closest('.highlight');
        if (container) {
            container.style.position = 'relative';
            container.appendChild(button);

            container.addEventListener('mouseenter', () => button.style.opacity = '1');
            container.addEventListener('mouseleave', () => button.style.opacity = '0');

            button.addEventListener('click', function() {
                const text = codeBlock.textContent;
                navigator.clipboard.writeText(text).then(function() {
                    button.textContent = 'Copied!';
                    setTimeout(() => button.textContent = 'Copy', 2000);
                });
            });
        }
    });
}

// 為 API 區塊添加摺疊功能
function addCollapsibleSections() {
    const apiSections = document.querySelectorAll('.api-section');

    apiSections.forEach(function(section) {
        const header = section.querySelector('h2, h3, h4');
        if (header) {
            header.style.cursor = 'pointer';
            header.style.userSelect = 'none';

            const toggle = document.createElement('span');
            toggle.textContent = ' ▼';
            toggle.style.fontSize = '0.8em';
            toggle.style.color = '#3498db';
            header.appendChild(toggle);

            const content = Array.from(section.children).slice(1);

            header.addEventListener('click', function() {
                const isCollapsed = content[0].style.display === 'none';

                content.forEach(function(element) {
                    element.style.display = isCollapsed ? '' : 'none';
                });

                toggle.textContent = isCollapsed ? ' ▼' : ' ▶';
            });
        }
    });
}

// 添加回到頂部按鈕
function addBackToTop() {
    const button = document.createElement('button');
    button.innerHTML = '↑';
    button.className = 'back-to-top';
    button.style.cssText = `
        position: fixed;
        bottom: 20px;
        right: 20px;
        width: 50px;
        height: 50px;
        background: #3498db;
        color: white;
        border: none;
        border-radius: 50%;
        font-size: 20px;
        cursor: pointer;
        display: none;
        z-index: 1000;
        transition: all 0.3s;
        box-shadow: 0 2px 10px rgba(0,0,0,0.3);
    `;

    document.body.appendChild(button);

    window.addEventListener('scroll', function() {
        if (window.pageYOffset > 300) {
            button.style.display = 'block';
        } else {
            button.style.display = 'none';
        }
    });

    button.addEventListener('click', function() {
        window.scrollTo({ top: 0, behavior: 'smooth' });
    });

    button.addEventListener('mouseenter', function() {
        this.style.transform = 'scale(1.1)';
        this.style.background = '#2980b9';
    });

    button.addEventListener('mouseleave', function() {
        this.style.transform = 'scale(1)';
        this.style.background = '#3498db';
    });
}

// 為外部連結添加圖示
function addExternalLinkIcons() {
    const externalLinks = document.querySelectorAll('a[href^="http"]:not([href*="' + window.location.hostname + '"])');

    externalLinks.forEach(function(link) {
        if (!link.querySelector('.external-icon')) {
            const icon = document.createElement('span');
            icon.className = 'external-icon';
            icon.innerHTML = ' ↗';
            icon.style.fontSize = '0.8em';
            icon.style.color = '#3498db';
            link.appendChild(icon);
        }
    });
}

// 程式碼語法高亮增強（如果需要）
function enhanceCodeBlocks() {
    const codeBlocks = document.querySelectorAll('code[class*="language-"]');

    codeBlocks.forEach(function(block) {
        // 可以在這裡添加額外的語法高亮邏輯
        block.addEventListener('dblclick', function() {
            // 雙擊選擇整個程式碼區塊
            const range = document.createRange();
            range.selectNodeContents(this);
            const selection = window.getSelection();
            selection.removeAllRanges();
            selection.addRange(range);
        });
    });
}