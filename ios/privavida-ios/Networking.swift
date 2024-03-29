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

    var websockets: Array<WebSocketHandle?> = []

    @objc func websocketOpen(url: UnsafePointer<Int8>?, userData: UnsafeMutableRawPointer?) -> Int32 {
        guard let url = url else {
            return -1
        }

        let url2 = URL(string: String(cString: url))
        guard let url2 = url2 else {
            return -1
        }

        print("WebSocket open: \(String(cString: url))")
        let wsId = Int32(self.websockets.count)
        let wsHandle = WebSocketHandle(url: url2, id: wsId, userData: userData)
        self.websockets.append(wsHandle)
        return wsId
    }

    @objc func websocketSend(ws: Int32, data: UnsafePointer<Int8>?) {
        if ws < 0 || ws >= self.websockets.count {
            return
        }
        guard let wsHandle = self.websockets[Int(ws)], let data = data else {
            return
        }

        let message = String(cString: data)
        wsHandle.socket.write(string: message)
    }

    @objc func websocketClose(ws: Int32, code: UInt16, reason: UnsafePointer<Int8>?) {
        if ws < 0 || ws >= self.websockets.count {
            return
        }
        guard let wsHandle = self.websockets[Int(ws)], let reason = reason else {
            return
        }

        let message = String(cString: reason)
        print("WebSocket close: code=\(code), reason=\(message)")
        wsHandle.socket.disconnect(closeCode: code)
    }

    func websocketRemove(ws: Int32) {
        if ws < 0 || ws >= self.websockets.count {
            return
        }
        self.websockets[Int(ws)] = nil
    }

    @objc func httpRequest(url: UnsafePointer<Int8>?, userData: UnsafeMutableRawPointer?) {
        guard let url = url else {
            return
        }

        let url2 = URL(string: String(cString: url))
        guard let url2 = url2 else {
            return
        }

        let request = URLRequest(url: url2)

        let task = URLSession.shared.dataTask(with: request) { (data, response, error) in

            if error != nil {
                app_http_response(-1, nil, 0, userData)
                return
            }

            guard let httpResponse = response as? HTTPURLResponse else {
                app_http_response(-1, nil, 0, userData)
                return
            }

            let statusCode = Int32(httpResponse.statusCode)

            guard let responseData = data else {
                app_http_response(statusCode, nil, 0, userData)
                return
            }

            responseData.withUnsafeBytes { ptr in
                let data = unsafeBitCast(ptr.baseAddress, to: UnsafePointer<UInt8>.self)
                let dataLength = Int32(responseData.count)
                app_http_response(statusCode, data, dataLength, userData)
            }
        }

        task.resume()
    }

    @objc func httpRequestImage(url: UnsafePointer<Int8>?, userData: UnsafeMutableRawPointer?) {
        guard let url = url else {
            return
        }

        let url2 = URL(string: String(cString: url))
        guard let url2 = url2 else {
            return
        }

        let request = URLRequest(url: url2)

        let task = URLSession.shared.dataTask(with: request) { (data, response, error) in

            if error != nil {
                app_http_response_for_image(-1, nil, 0, userData)
                return
            }

            guard let httpResponse = response as? HTTPURLResponse else {
                app_http_response_for_image(-1, nil, 0, userData)
                return
            }

            let statusCode = Int32(httpResponse.statusCode)

            guard let responseData = data else {
                app_http_response_for_image(statusCode, nil, 0, userData)
                return
            }

            responseData.withUnsafeBytes { ptr in
                let data = unsafeBitCast(ptr.baseAddress, to: UnsafePointer<UInt8>.self)
                let dataLength = Int32(responseData.count)
                app_http_response_for_image(statusCode, data, dataLength, userData)
            }
        }

        task.resume()
    }

}

class WebSocketHandle: WebSocketDelegate {
    let socket: WebSocket
    let id: Int32
    let userData: UnsafeMutableRawPointer?
    let url: URL

    init(url: URL, id: Int32, userData: UnsafeMutableRawPointer?) {
        self.url = url
        self.userData = userData
        self.id = id
        self.socket = WebSocket(request: URLRequest(url: url), compressionHandler: .none)
        self.socket.delegate = self
        self.socket.connect()
    }

    public func didReceive(event: WebSocketEvent, client: WebSocket) {
        switch event {
        case .connected:
            var event = AppWebsocketEvent()
            event.type = WEBSOCKET_OPEN
            event.socket = self.id
            event.user_data = userData
            app_websocket_event(&event)

        case .disconnected:
            var event = AppWebsocketEvent()
            event.type = WEBSOCKET_CLOSE
            event.socket = self.id
            event.user_data = userData
            app_websocket_event(&event)
            Networking.sharedInstance.websocketRemove(ws: self.id)

        case .cancelled, .error:
            var event = AppWebsocketEvent()
            event.type = WEBSOCKET_ERROR
            event.socket = self.id
            event.user_data = userData
            app_websocket_event(&event)
            Networking.sharedInstance.websocketRemove(ws: self.id)

        case .text(let txt):
            let cString = txt.utf8CString
            cString.withUnsafeBytes { ptr in
                var event = AppWebsocketEvent()
                event.type = WEBSOCKET_MESSAGE
                event.socket = self.id
                event.user_data = userData
                event.data = unsafeBitCast(ptr.baseAddress, to: UnsafePointer<Int8>.self)
                event.data_length = Int32(strlen(event.data))
                app_websocket_event(&event)
            }

        default:
            break
        }
    }
}
