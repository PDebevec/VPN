window.addEventListener('DOMContentLoaded', () => {
    document.head.appendChild(
        Object.assign(document.createElement('meta'), {
            httpEquiv: 'Content-Security-Policy',
            content: "script-src 'self' 'unsafe-inline' https://cdn.jsdelivr.net; style-src 'self' 'unsafe-inline' https://cdn.jsdelivr.net;"
        })
    );
    document.head.appendChild(
        Object.assign(document.createElement('link'), {
            rel: 'stylesheet',
            href: '../assets/style/css.css'
        })
    );
    document.head.appendChild(
        Object.assign(document.createElement('script'), {
            src: '../assets/script/js.js'
        })
    );
})