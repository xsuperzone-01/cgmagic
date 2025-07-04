package xhttp

import (
	"context"
	"fmt"
	"io"
	"net/http"
)

func DownloadW(ctx context.Context, url string, writer io.Writer, start int64) error {
	req, err := http.NewRequest(http.MethodGet, url, nil)
	if err != nil {
		return err
	}
	if ctx != nil {
		req = req.WithContext(ctx)
	}
	req.Header.Add("Range", fmt.Sprintf("bytes=%d-", start))
	client := HttpClient()
	res, err := client.Do(req)
	if err != nil {
		return err
	}
	defer res.Body.Close()

	_, err = io.Copy(writer, res.Body)
	if err != nil {
		return err
	}
	return nil
}

func DownloadR(ctx context.Context, url string, start int64) (error, io.ReadCloser, http.Header) {
	req, err := http.NewRequest(http.MethodGet, url, nil)
	if err != nil {
		return err, nil, nil
	}
	if ctx != nil {
		req = req.WithContext(ctx)
	}
	req.Header.Add("Range", fmt.Sprintf("bytes=%d-", start))
	client := HttpClient()
	res, err := client.Do(req)

	if err != nil {
		return err, nil, nil
	}
	//close res.Body by caller
	return nil, res.Body, res.Header
}