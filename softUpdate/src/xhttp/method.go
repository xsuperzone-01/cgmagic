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
	client := http.Client{}
	res, err := client.Do(req)
	defer res.Body.Close()
	if err != nil {
		return err
	}
	_, err = io.Copy(writer, res.Body)
	if err != nil {
		return err
	}
	return nil
}
