function changeIframe(that, side) {
    document.getElementById('content').setAttribute('src', side.split('-')[1] + '.html')
    if (side != undefined) {
        document.getElementById('content').setAttribute('name', side.split('-')[0])
    } else {
        document.getElementById('content').setAttribute('name', '')
    }
}