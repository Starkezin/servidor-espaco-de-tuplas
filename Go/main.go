package main

import (
	"bufio"
	"fmt"
	"net"
	"strings"
	"sync"
)

type Tuple struct {
	key   string
	value string
}

type TupleSpace struct {
	mu      sync.RWMutex
	data    map[string][]string      
	waiters map[string][]chan string
}

func NewTupleSpace() *TupleSpace {
	return &TupleSpace{
		data:    make(map[string][]string),
		waiters: make(map[string][]chan string),
	}
}

// Serviços Registrados
var services = map[string]func(string) string{
	"1": func(v string) string { return strings.ToUpper(v) },        // Maiúsculas 
	"2": func(v string) string { return reverseString(v) },          // Inverter 
	"3": func(v string) string { return fmt.Sprintf("%d", len(v)) }, // Tamanho
}

func reverseString(s string) string {
	runes := []rune(s)
	for i, j := 0, len(runes)-1; i < j; i, j = i+1, j-1 {
		runes[i], runes[j] = runes[j], runes[i]
	}
	return string(runes)
}

//  Operações 

//  WR
func (ts *TupleSpace) Write(key, value string) {
	ts.mu.Lock()
	defer ts.mu.Unlock()

	if waiters, ok := ts.waiters[key]; ok && len(waiters) > 0 {
		ch := waiters[0]
		ts.waiters[key] = waiters[1:]
		ch <- value
		return
	}

	ts.data[key] = append(ts.data[key], value)
}

// IN 
func (ts *TupleSpace) Take(key string) string {
	ts.mu.Lock()

	if vals, ok := ts.data[key]; ok && len(vals) > 0 {
		val := vals[0]
		ts.data[key] = vals[1:]
		ts.mu.Unlock()
		return val
	}

	ch := make(chan string)
	ts.waiters[key] = append(ts.waiters[key], ch)
	ts.mu.Unlock()
	return <-ch
}

//  RD
func (ts *TupleSpace) Read(key string) string {
	ts.mu.Lock()
	if vals, ok := ts.data[key]; ok && len(vals) > 0 {
		val := vals[0]
		ts.mu.Unlock()
		return val
	}
	ts.mu.Unlock()

	val := ts.Take(key)
	ts.Write(key, val)
	return val
}

// Protocolo TCP e Handler 

func handleConnection(conn net.Conn, ts *TupleSpace) {
	defer conn.Close()
	scanner := bufio.NewScanner(conn)

	for scanner.Scan() {
		line := scanner.Text()
		parts := strings.Fields(line)
		if len(parts) == 0 {
			continue
		}

		command := strings.ToUpper(parts[0])
		var response string

		switch command {
		case "WR": 
			if len(parts) >= 3 {
				ts.Write(parts[1], strings.Join(parts[2:], " "))
				response = "OK"
			} else {
				response = "ERROR"
			}

		case "RD": 
			if len(parts) == 2 {
				val := ts.Read(parts[1])
				response = "OK " + val
			} else {
				response = "ERROR"
			}

		case "IN": 
			if len(parts) == 2 {
				val := ts.Take(parts[1])
				response = "OK " + val
			} else {
				response = "ERROR"
			}

		case "EX": 
			if len(parts) == 4 {
				kIn, kOut, svcId := parts[1], parts[2], parts[3]
				svc, exists := services[svcId]
				if !exists {
					response = "NO-SERVICE"
				} else {
					go func() {
						vIn := ts.Take(kIn)
						vOut := svc(vIn)
						ts.Write(kOut, vOut)
					}()
					response = "OK"
				}
			} else {
				response = "ERROR"
			}

		default:
			response = "ERROR"
		}

		fmt.Fprintln(conn, response)
	}
}

func main() {
	port := "54321" 
	ts := NewTupleSpace()

	ln, err := net.Listen("tcp", ":"+port)
	if err != nil {
		fmt.Printf("Erro ao iniciar servidor: %v\n", err)
		return
	}
	fmt.Printf("Servidor Linda (Go) rodando na porta %s...\n", port)

	for {
		conn, err := ln.Accept()
		if err != nil {
			continue
		}
		go handleConnection(conn, ts)
	}
}
