//
//  Networking.swift
//  privavida-ios
//
//  Created by Bartholomew Joyce on 20/05/2023.
//

import Foundation
import Starscream

@objc public class Networking: NSObject {
    @objc static let sharedInstance = Networking()

    @objc var appNetworking: AppNetworking {
        var net = AppNetworking()
        net.websocket_open = { url, user_data in
            return Int32(Networking.sharedInstance.websocketOpen(url: url, userData: user_data))
        }
        net.websocket_send = { ws, data in
            Networking.sharedInstance.websocketSend(ws: ws, data: data)
        }
        net.websocket_close = { ws, code, reason in
            Networking.sharedInstance.websocketClose(ws: ws, code: code, reason: reason)
        }
        net.http_request_send = { url, user_data in
            Networking.sharedInstance.httpRequestSend(url: url, userData: user_data)
        }
        return net
    }

    var wsHandle: WebSocketHandle?
    
    func websocketOpen(url: UnsafePointer<Int8>?, userData: UnsafeMutableRawPointer?) -> Int32 {
        guard let url = url else {
            return -1
        }

        let url2 = URL(string: String(cString: url))
        guard let url2 = url2 else {
            return -1
        }

        print("WebSocket open: \(String(cString: url))")
        wsHandle = WebSocketHandle(url: url2, handle: 1)
        return 1
    }

    func websocketSend(ws: Int32, data: UnsafePointer<Int8>?) {
        guard let data = data else {
            return
        }
        let message = String(cString: data)
        wsHandle!.socket.write(string: message)
    }

    func websocketClose(ws: Int32, code: UInt16, reason: UnsafePointer<Int8>?) {
        guard let reason = reason else {
            return
        }
        let message = String(cString: reason)
        print("WebSocket close: code=\(code), reason=\(message)")
    }
    
    func httpRequestSend(url: UnsafePointer<Int8>?, userData: UnsafeMutableRawPointer?) {
        guard let url = url else {
            return
        }

        let url2 = URL(string: String(cString: url))
        guard let url2 = url2 else {
            return
        }

        let request = URLRequest(url: url2)

        let task = URLSession.shared.dataTask(with: request) { (data, response, error) in
            var event = AppHttpEvent()
            event.data_length = 0;
            event.data = nil;
            event.user_data = userData;

            if let error = error {
                event.type = HTTP_RESPONSE_ERROR;
                event.status_code = -1;
                app_http_event(&event)
                return
            }

            guard let httpResponse = response as? HTTPURLResponse else {
                event.type = HTTP_RESPONSE_ERROR;
                event.status_code = -1;
                app_http_event(&event)
                return
            }

            event.status_code = Int32(httpResponse.statusCode)

            if let responseData = data {
                responseData.withUnsafeBytes { ptr in
                    var dataEvent = event
                    dataEvent.type = HTTP_RESPONSE_DATA
                    dataEvent.data_length = Int32(responseData.count)
                    dataEvent.data = unsafeBitCast(ptr.baseAddress, to: UnsafePointer<UInt8>.self)
                    app_http_event(&dataEvent)
                }
            }

            event.type = HTTP_RESPONSE_END
            app_http_event(&event)
        }

        task.resume()
    }

}

class WebSocketHandle: WebSocketDelegate {
    lazy var socket = {
        let req = URLRequest(url: url)
        let socket = WebSocket(request: req, compressionHandler: .none)
        socket.delegate = self
        return socket
    }()
    let handle: Int32
    let url: URL

    init(url: URL, handle: Int32) {
        self.url = url
        self.handle = handle
        self.socket.connect()
    }
    
    public func didReceive(event: WebSocketEvent, client: WebSocket) {
        switch event {
        case .connected:
            print("WebSocket connected")
            var event = AppWebsocketEvent()
            event.type = WEBSOCKET_OPEN
            event.ws = handle
            app_websocket_event(&event)

        case .disconnected:
            print("WebSocket disconnected")
            var event = AppWebsocketEvent()
            event.type = WEBSOCKET_CLOSE
            event.ws = handle
            app_websocket_event(&event)

        case .cancelled, .error:
            print("WebSocket error")
            var event = AppWebsocketEvent()
            event.type = WEBSOCKET_ERROR
            event.ws = handle
            app_websocket_event(&event)

        case .text(let txt):
            let cString = txt.utf8CString
            cString.withUnsafeBytes { ptr in
                var event = AppWebsocketEvent()
                event.type = WEBSOCKET_MESSAGE
                event.ws = handle
                event.data = unsafeBitCast(ptr.baseAddress, to: UnsafePointer<Int8>.self)
                event.data_length = Int32(strlen(event.data))
                app_websocket_event(&event)
            }

        default:
            break
        }
    }
}
