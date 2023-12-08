function handleRequest(url) {
    if (url === "/john") {
        return "Hello John Keng!";
    } else {
        return "Hello from JS!";
    }
}

globalThis.handleRequest = handleRequest;
